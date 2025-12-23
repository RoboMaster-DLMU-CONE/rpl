/**
 * @file Parser.hpp
 * @brief RPL库的解析器实现
 *
 * 此文件包含Parser类的定义，该类用于解析流式数据包。
 * 支持分片接收、噪声容错和并发多包处理。
 *
 * @author WindWeaver
 */

#ifndef RPL_PARSER_HPP
#define RPL_PARSER_HPP

#include <algorithm>
#include <array>
#include <bit>
#include <cppcrc.h>
#include <optional>

#include "Containers/RingBuffer.hpp"
#include "Deserializer.hpp"
#include "Meta/PacketTraits.hpp"
#include "Utils/Def.hpp"
#include "Utils/Error.hpp"
#include <tl/expected.hpp>

namespace RPL {
/**
 * @brief 解析器类
 *
 * 用于解析流式数据包，支持分片接收、噪声容错和并发多包处理。
 * 使用环形缓冲区来处理流式数据，并通过CRC校验确保数据完整性。
 *
 * @tparam Ts 可解析的数据包类型列表
 */
template <typename... Ts> class Parser {
  static constexpr size_t max_frame_size = std::max({(
      FRAME_HEADER_SIZE + Meta::PacketTraits<Ts>::size + FRAME_TAIL_SIZE)...});

  /**
   * @brief 编译期计算缓冲区大小
   *
   * 计算最优的环形缓冲区大小，为4倍最大帧大小且为2的幂
   *
   * @return 计算出的缓冲区大小
   */
  static consteval size_t calculate_buffer_size() {
    constexpr size_t min_size = max_frame_size * 4;

    if constexpr (std::has_single_bit(min_size)) {
      return min_size;
    } else {
      return std::bit_ceil(min_size);
    }
  }

  static constexpr size_t buffer_size =
      calculate_buffer_size(); ///< 环形缓冲区大小
  Containers::RingBuffer<buffer_size>
      ringbuffer; ///< 环形缓冲区，用于存储接收的数据
  std::array<uint8_t, max_frame_size> parse_buffer{}; ///< 临时解析缓冲区

  // 直接引用 Deserializer
  Deserializer<Ts...> &deserializer;

public:
  /**
   * @brief 构造函数
   *
   * 使用反序列化器引用构造解析器
   *
   * @param des 反序列化器引用，用于存储解析后的数据
   */
  explicit Parser(Deserializer<Ts...> &des) : deserializer(des) {}

  /**
   * @brief 推送数据到解析器
   *
   * 将接收到的字节数据推送到解析器，并尝试解析数据包
   *
   * @param buffer 包含数据的缓冲区
   * @param length 数据长度
   * @return 成功时返回void，失败时返回错误信息
   */
  tl::expected<void, Error> push_data(const uint8_t *buffer,
                                      const size_t length) {
    if (!ringbuffer.write(buffer, length)) {
      return tl::unexpected(
          Error{ErrorCode::BufferOverflow, "Ringbuffer overflow"});
    }

    return try_parse_packets();
  }

  /**
   * @brief 获取可直接写入的缓冲区视图
   *
   * 获取RingBuffer内部的写指针，用于零拷贝写入（如DMA）。
   * 写入完成后必须调用 advance_write_index。
   *
   * @return 包含指针和最大可写入长度的span
   */
  std::span<uint8_t> get_write_buffer() noexcept {
    return ringbuffer.get_write_buffer();
  }

  /**
   * @brief 更新写索引
   *
   * 在直接写入缓冲区后调用此函数。
   *
   * @param length 实际写入的字节数
   * @return 成功时返回void，失败时返回错误信息
   */
  tl::expected<void, Error> advance_write_index(size_t length) {
    if (!ringbuffer.advance_write_index(length)) {
      return tl::unexpected(
          Error{ErrorCode::BufferOverflow, "Invalid advance length"});
    }
    return try_parse_packets();
  }

  /**
   * @brief 获取反序列化器引用
   *
   * 获取与解析器关联的反序列化器引用
   *
   * @return 反序列化器引用
   */
  Deserializer<Ts...> &get_deserializer() noexcept { return deserializer; }

  /**
   * @brief 获取可用数据量
   *
   * 获取环形缓冲区中当前可用的数据量
   *
   * @return 可用数据量（字节）
   */
  size_t available_data() const noexcept { return ringbuffer.available(); }

  /**
   * @brief 获取可用空间
   *
   * 获取环形缓冲区中当前可用的空间
   *
   * @return 可用空间（字节）
   */
  size_t available_space() const noexcept { return ringbuffer.space(); }

  /**
   * @brief 检查缓冲区是否已满
   *
   * 检查环形缓冲区是否已满
   *
   * @return 如果缓冲区已满返回true，否则返回false
   */
  bool is_buffer_full() const noexcept { return ringbuffer.full(); }

  /**
   * @brief 清空缓冲区
   *
   * 清空环形缓冲区中的所有数据
   */
  void clear_buffer() noexcept { ringbuffer.clear(); }

  /**
   * @brief 尝试解析数据包
   *
   * 从环形缓冲区中尝试解析数据包，优先使用快速路径（零拷贝），
   * 如果数据跨越边界则自动回退到分段处理路径。
   *
   * @return 成功时返回void，失败时返回错误信息
   */
  tl::expected<void, Error> try_parse_packets() {
    // 获取缓冲区状态
    size_t available_bytes = ringbuffer.available();

    while (available_bytes >= FRAME_HEADER_SIZE) {
      const auto buffer_view = ringbuffer.get_contiguous_read_buffer();
      const uint8_t *data_ptr = buffer_view.data();
      const size_t view_size = buffer_view.size();

      // 快速扫描起始字节
      const uint8_t *start_ptr = nullptr;
      size_t scan_offset = 0;

      // 在当前连续块中搜索
      while (scan_offset < view_size) {
        start_ptr = static_cast<const uint8_t *>(std::memchr(
            data_ptr + scan_offset, FRAME_START_BYTE, view_size - scan_offset));

        if (!start_ptr) {
          // 当前连续块中没找到，丢弃这部分数据
          // 注意：如果这是第一块且还有第二块（Wrap Around），我们只丢弃第一块
          // 下次循环会处理第二块
          ringbuffer.discard(view_size - scan_offset);
          available_bytes -= (view_size - scan_offset);
          goto next_iteration;
        }

        const size_t start_offset = start_ptr - data_ptr;

        // 丢弃起始位置之前的垃圾数据
        if (start_offset > 0) {
          ringbuffer.discard(start_offset);
          available_bytes -= start_offset;
          // 更新指针和大小
        }

        // 尝试解析帧
        // 此时 ringbuffer 的 read_index 指向 FRAME_START_BYTE
        ParseResult result = parse_frame();

        if (result == ParseResult::Success) {
          // 解析成功，parse_frame 内部已经丢弃了整个帧
          available_bytes = ringbuffer.available();
          // 重新获取 view，因为 read_index 变了
          break;
        } else if (result == ParseResult::Failure) {
          // 解析失败（CRC错误或长度错误），丢弃1字节并继续搜索
          ringbuffer.discard(1);
          available_bytes--;
          // 更新 scan_offset 继续在当前 view 中搜索
          scan_offset = start_offset + 1;
        } else {
          // ParseResult::Incomplete
          // 数据不足，停止解析，等待更多数据
          // 注意：这里不能丢弃任何数据，也不能继续搜索
          // 必须直接返回，因为 RingBuffer 是 FIFO
          // 的，队头不完整无法处理后续数据
          return {};
        }
      }

    next_iteration:
      if (available_bytes < FRAME_HEADER_SIZE) {
        break;
      }
    }

    return {};
  }

private:
  enum class ParseResult { Success, Failure, Incomplete };

  /**
   * @brief 解析单个帧
   *
   * 尝试从当前 read_index 解析一个完整的帧。
   * 处理了数据跨越 RingBuffer 边界的情况。
   *
   * @return 解析结果状态
   */
  ParseResult parse_frame() {
    // 1. 读取并验证帧头
    std::array<uint8_t, FRAME_HEADER_SIZE> header_buffer;

    // 尝试直接从连续内存读取帧头（快速路径）
    auto view = ringbuffer.get_contiguous_read_buffer();
    const uint8_t *header_ptr;

    if (view.size() >= FRAME_HEADER_SIZE) {
      header_ptr = view.data();
    } else {
      // 慢速路径：帧头跨界
      header_ptr = header_buffer.data();
    }

    // 验证帧头 CRC8
    if (header_ptr[0] != FRAME_START_BYTE)
      return ParseResult::Failure;

    const uint8_t received_crc8 = header_ptr[6];
    if (CRC8::CRC8::calc(header_ptr, 6) != received_crc8)
      return ParseResult::Failure;

    // 提取元数据
    uint16_t cmd;
    uint16_t data_length;
#if defined(__ARM_FEATURE_UNALIGNED) || defined(__i386__) || defined(__x86_64__)
    cmd = *reinterpret_cast<const uint16_t *>(header_ptr + 1);
    data_length = *reinterpret_cast<const uint16_t *>(header_ptr + 3);
#else
    std::memcpy(&cmd, header_ptr + 1, sizeof(uint16_t));
    std::memcpy(&data_length, header_ptr + 3, sizeof(uint16_t));
#endif

    // 检查长度
    if (data_length > max_frame_size - FRAME_HEADER_SIZE - FRAME_TAIL_SIZE)
      return ParseResult::Failure;

    const size_t complete_frame_size =
        FRAME_HEADER_SIZE + data_length + FRAME_TAIL_SIZE;
    if (ringbuffer.available() < complete_frame_size)
      return ParseResult::Incomplete; // 数据不足

    // 2. 验证 CRC16
    const size_t crc16_data_len = complete_frame_size - FRAME_TAIL_SIZE;
    uint16_t calculated_crc16;

    if (view.size() >= crc16_data_len) {
      // 快速路径：整个 CRC 计算区域都在连续内存中
      calculated_crc16 = CRC16::CCITT_FALSE::calc(view.data(), crc16_data_len);
    } else {
      // 慢速路径：分段计算
      // 第一段
      uint16_t crc_part1 = CRC16::CCITT_FALSE::calc(view.data(), view.size());

      // 第二段
      const size_t second_part_len = crc16_data_len - view.size();
      // 借用 parse_buffer 暂存第二段数据用于计算 CRC
      // 注意：这里假设 parse_buffer 足够大，max_frame_size 保证了这一点

      calculated_crc16 = CRC16::CCITT_FALSE::calc(parse_buffer.data(),
                                                  second_part_len, crc_part1);
    }

    // 读取接收到的 CRC16
    uint16_t received_crc16;
    if (!ringbuffer.peek(reinterpret_cast<uint8_t *>(&received_crc16),
                         crc16_data_len, sizeof(uint16_t)))
      return ParseResult::Incomplete;

    if (calculated_crc16 != received_crc16)
      return ParseResult::Failure;

    // 3. 提取数据
    ringbuffer.discard(FRAME_HEADER_SIZE);

    uint8_t *write_ptr = deserializer.getWritePtr(cmd);
    if (write_ptr != nullptr) {
      ringbuffer.read(write_ptr, data_length);
    } else {
      ringbuffer.discard(data_length);
    }

    ringbuffer.discard(FRAME_TAIL_SIZE);
    return ParseResult::Success;
  }

  /**
   * @brief 验证帧头
   *
   * 验证给定缓冲区中的帧头是否有效
   *
   * @param header 指向帧头的指针
   * @return 如果帧头有效返回包含命令码、数据长度和序列号的元组，否则返回nullopt
   */
  static std::optional<std::tuple<uint16_t, uint16_t, uint8_t>>
  validate_header(const uint8_t *header) {
    if (header[0] != FRAME_START_BYTE) {
      return std::nullopt;
    }

    const uint16_t cmd = *reinterpret_cast<const uint16_t *>(header + 1);
    const uint16_t data_length =
        *reinterpret_cast<const uint16_t *>(header + 3);
    const uint8_t sequence_number = header[5];
    const uint8_t received_crc8 = header[6];

    const uint8_t calculated_crc8 = CRC8::CRC8::calc(header, 6);
    if (calculated_crc8 != received_crc8) {
      return std::nullopt;
    }

    return std::make_tuple(cmd, data_length, sequence_number);
  }
};
} // namespace RPL

#endif // RPL_PARSER_HPP
