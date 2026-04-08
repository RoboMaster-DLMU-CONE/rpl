/**
 * @file Parser.hpp
 * @brief RPL库的解析器实现
 *
 * 此文件包含Parser类的定义，该类用于解析流式数据包。
 * 支持分片接收、噪声容错和并发多包处理。
 * 支持可选的连接健康检测功能。
 *
 * @author WindWeaver
 */

#ifndef RPL_PARSER_HPP
#define RPL_PARSER_HPP

#include "Containers/BipBuffer.hpp"
#include "Deserializer.hpp"
#include "Meta/PacketTraits.hpp"
#include "Utils/ConnectionMonitor.hpp"
#include "Utils/Def.hpp"
#include "Utils/Error.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <cstring>
#include <optional>
#include <tl/expected.hpp>
#include <tuple>
#include <type_traits>

/**
 * @namespace RPL
 * @brief RoboMaster Packet Library 的主命名空间
 */
namespace RPL {

extern void RPL_ERROR_START_BYTE_COLLISION();

namespace Details {
// --- 类型去重工具 ---

/**
 * @brief 类型列表
 * @tparam Ts 类型列表
 */
template <typename... Ts> struct TypeList {};

/**
 * @brief 检查类型是否在列表中
 * @tparam T 要查找的类型
 * @tparam List 类型列表
 */
template <typename T, typename List> struct Contains;
template <typename T, typename... Ts>
struct Contains<T, TypeList<Ts...>> : std::disjunction<std::is_same<T, Ts>...> {
};

template <typename In, typename Out> struct UniqueImpl;
template <typename Out> struct UniqueImpl<TypeList<>, Out> {
  using type = Out;
};
template <typename H, typename... Ts, typename... Os>
struct UniqueImpl<TypeList<H, Ts...>, TypeList<Os...>> {
  using type = std::conditional_t<
      Contains<H, TypeList<Os...>>::value,
      typename UniqueImpl<TypeList<Ts...>, TypeList<Os...>>::type,
      typename UniqueImpl<TypeList<Ts...>, TypeList<Os..., H>>::type>;
};

template <typename... Ts>
using UniqueTypes_t = typename UniqueImpl<TypeList<Ts...>, TypeList<>>::type;

/**
 * @brief 运行时获取 Tuple 元素
 *
 * 根据索引在运行时选择 tuple 元素并调用函数。
 * 用于在运行时根据协议索引分发到编译期生成的 Worker。
 *
 * @tparam Tuple Tuple 类型
 * @tparam Fn 函数对象类型
 * @param i 索引
 * @param t Tuple 实例
 * @param f 要调用的函数
 */
template <typename Tuple, typename Fn, size_t... Is>
constexpr void tuple_switch(size_t i, Tuple &&t, Fn &&f,
                            std::index_sequence<Is...>) {
  ((i == Is ? f(std::get<Is>(t)) : void()), ...);
}

/**
 * @brief 便捷函数：运行时获取 Tuple 元素
 * @tparam Tuple Tuple 类型
 * @tparam Fn 函数对象类型
 * @param i 索引
 * @param t Tuple 实例
 * @param f 要调用的函数
 */
template <typename Tuple, typename Fn>
constexpr void runtime_get(size_t i, Tuple &&t, Fn &&f) {
  tuple_switch(i, std::forward<Tuple>(t), std::forward<Fn>(f),
               std::make_index_sequence<
                   std::tuple_size_v<std::remove_reference_t<Tuple>>>{});
}

// --- ConnectionMonitor 检测工具 ---

/**
 * @brief 检查类型是否有 PacketTraits 特化 (即是否是数据包类型)
 * @tparam T 要检查的类型
 */
template <typename T>
concept IsPacketType = requires {
  { Meta::PacketTraits<T>::cmd } -> std::convertible_to<uint16_t>;
  { Meta::PacketTraits<T>::size } -> std::convertible_to<size_t>;
};

/**
 * @brief 检查类型是否是 ConnectionMonitor (满足 concept 且不是 Packet)
 * @tparam T 要检查的类型
 */
template <typename T>
struct IsConnectionMonitor
    : std::bool_constant<ConnectionMonitorConcept<T> && !IsPacketType<T>> {};

// 从模板参数中提取 Monitor 和 Packets
template <typename... Args> struct ExtractMonitorAndPackets;

// 第一个参数是 ConnectionMonitor
template <typename M, typename... Ps>
  requires IsConnectionMonitor<M>::value
struct ExtractMonitorAndPackets<M, Ps...> {
  using Monitor = M;
  using Packets = TypeList<Ps...>;
};

// 第一个参数不是 ConnectionMonitor (全部是 Packet)
template <typename... Ps>
  requires(
      sizeof...(Ps) > 0 &&
      !IsConnectionMonitor<std::tuple_element_t<0, std::tuple<Ps...>>>::value)
struct ExtractMonitorAndPackets<Ps...> {
  using Monitor = NullConnectionMonitor;
  using Packets = TypeList<Ps...>;
};

// 空参数列表
template <> struct ExtractMonitorAndPackets<> {
  using Monitor = NullConnectionMonitor;
  using Packets = TypeList<>;
};
} // namespace Details

/**
 * @brief 解析器类
 *
 * 用于解析流式数据包，支持多协议混合解析。
 * 根据 PacketTraits 中定义的 Protocol 自动生成查找表和解析逻辑。
 * 支持可选的连接监控功能。
 *
 * @tparam Args 模板参数列表，可以是:
 *              - 仅数据包类型: Parser<PacketA, PacketB>
 *              - ConnectionMonitor + 数据包类型: Parser<Monitor, PacketA,
 * PacketB>
 *
 * @code
 * // 方式1: 无监控 (零开销)
 * RPL::Parser<SampleA, SampleB> parser{deserializer};
 *
 * // 方式2: 使用内置 Tick 监控器
 * struct HALTickProvider {
 *     using tick_type = uint32_t;
 *     static tick_type now() { return HAL_GetTick(); }
 * };
 * using Monitor = RPL::TickConnectionMonitor<HALTickProvider>;
 * RPL::Parser<Monitor, SampleA, SampleB> parser{deserializer};
 *
 * if (!parser.get_connection_monitor().is_connected(100)) {
 *     // 超过 100ms 未收到数据
 * }
 * @endcode
 */
template <typename... Args> class Parser {
  // 提取 Monitor 和 Packet 类型
  using Extracted = Details::ExtractMonitorAndPackets<Args...>;
  using MonitorType = typename Extracted::Monitor;

  // 从 TypeList 展开 Packet 类型的辅助模板
  template <typename PacketList> struct ParserImpl;

  template <typename... Ts> struct ParserImpl<Details::TypeList<Ts...>> {
    // --- 解析 Worker ---
    template <typename P, bool IsFixed, uint16_t CmdId, size_t DataSize>
    struct ProtocolWorker {
      using Protocol = P;
      static constexpr bool is_fixed = IsFixed;
      static constexpr uint16_t fixed_cmd = CmdId;
      static constexpr size_t fixed_size = DataSize;
      static constexpr size_t min_frame_size =
          P::header_size + (IsFixed ? DataSize : 0) + P::tail_size;
    };

    // --- 为每个 T 提取 Worker 类型 ---
    template <typename T> struct GetWorker {
      using P = typename Meta::PacketTraits<T>::Protocol;
      static constexpr bool IsFixed = !P::has_cmd_field;
      static constexpr uint16_t C = IsFixed ? Meta::PacketTraits<T>::cmd : 0;
      static constexpr size_t S = IsFixed ? Meta::PacketTraits<T>::size : 0;
      using type = ProtocolWorker<P, IsFixed, C, S>;
    };

    // --- 生成去重后的 Worker 列表 ---
    template <typename TypeList> struct TupleFromList;
    template <typename... Ws> struct TupleFromList<Details::TypeList<Ws...>> {
      using type = std::tuple<Ws...>;
    };

    using AllWorkers = Details::TypeList<typename GetWorker<Ts>::type...>;
    using UniqueWorkers =
        Details::UniqueTypes_t<typename GetWorker<Ts>::type...>;
    using WorkerTuple = typename TupleFromList<UniqueWorkers>::type;

    // --- 编译期计算 Max Frame Size ---
    static constexpr size_t calculate_max_frame_size() {
      size_t max = 0;
      auto check = [&max]<typename T>() {
        using P = typename Meta::PacketTraits<T>::Protocol;
        size_t size =
            P::header_size + Meta::PacketTraits<T>::size + P::tail_size;
        if (size > max)
          max = size;
      };
      (check.template operator()<Ts>(), ...);
      return max;
    }

    static constexpr size_t max_frame_size = calculate_max_frame_size();

    // --- 计算 Buffer Size ---
    static consteval size_t calculate_buffer_size() {
      constexpr size_t min_size = max_frame_size * 4;
      if constexpr (std::has_single_bit(min_size))
        return min_size;
      else
        return std::bit_ceil(min_size);
    }

    static constexpr size_t buffer_size = calculate_buffer_size();

    // --- 构建查找表 ---
    static constexpr auto header_lut = []() {
      std::array<uint8_t, 256> table;
      table.fill(0xFF);

      auto register_worker = [&table]<typename W>(size_t index) {
        uint8_t sb = W::Protocol::start_byte;
        if (table[sb] != 0xFF && table[sb] != index) {
          RPL_ERROR_START_BYTE_COLLISION();
        }
        table[sb] = static_cast<uint8_t>(index);
      };

      auto helper =
          [&register_worker]<typename... Ws>(Details::TypeList<Ws...>) {
            size_t idx = 0;
            ((register_worker.template operator()<Ws>(idx++)), ...);
          };
      helper(UniqueWorkers{});

      return table;
    }();

    static constexpr uint8_t unique_start_byte = []() {
      uint8_t first_sb = 0xFF;
      size_t count = 0;
      for (int i = 0; i < 256; ++i) {
        if (header_lut[i] != 0xFF) {
          if (first_sb == 0xFF)
            first_sb = static_cast<uint8_t>(i);
          count++;
        }
      }
      return count == 1 ? first_sb : 0xFF;
    }();

    static constexpr bool has_multiple_start_bytes = []() {
      size_t count = 0;
      for (int i = 0; i < 256; ++i) {
        if (header_lut[i] != 0xFF)
          count++;
      }
      return count > 1;
    }();

    using DeserializerType = Deserializer<Ts...>;
  };

  using Impl = ParserImpl<typename Extracted::Packets>;
  using WorkerTuple = typename Impl::WorkerTuple;
  using UniqueWorkers = typename Impl::UniqueWorkers;

  static constexpr size_t max_frame_size = Impl::max_frame_size;
  static constexpr size_t buffer_size = Impl::buffer_size;
  static constexpr auto &header_lut = Impl::header_lut;
  static constexpr uint8_t unique_start_byte = Impl::unique_start_byte;
  static constexpr bool has_multiple_start_bytes = Impl::has_multiple_start_bytes;

  // 从 Packets TypeList 中提取 Deserializer 类型
  template <typename PacketList> struct DeserializerFromPackets;
  template <typename... Ts>
  struct DeserializerFromPackets<Details::TypeList<Ts...>> {
    using type = Deserializer<Ts...>;
  };

  using DeserializerType =
      typename DeserializerFromPackets<typename Extracted::Packets>::type;

  /**
   * @brief 解析结果枚举
   *
   * 表示单次解析尝试的结果状态。
   */
  enum class ParseResult { 
    Success,    ///< 成功解析一个完整帧
    Failure,    ///< 解析失败（校验错误等），需要丢弃数据并重试
    Incomplete  ///< 数据不完整，需要等待更多数据
  };

  // --- 成员变量 ---
  Containers::BipBuffer<buffer_size> buffer;
  DeserializerType &deserializer;
  [[no_unique_address]] MonitorType monitor_{};

public:
  explicit Parser(DeserializerType &des) : deserializer(des) {}

  /**
   * @brief 获取连接监控器引用
   *
   * @return 连接监控器的引用
   */
  MonitorType &get_connection_monitor() noexcept { return monitor_; }

  /**
   * @brief 获取连接监控器常量引用
   *
   * @return 连接监控器的常量引用
   */
  const MonitorType &get_connection_monitor() const noexcept {
    return monitor_;
  }

  /**
   * @brief 推送数据到解析器
   *
   * 将接收到的数据写入内部缓冲区，并尝试解析数据包。
   * 解析成功后，数据会自动从缓冲区移除。
   *
   * @param data 指向输入数据的指针
   * @param length 数据长度
   * @return void 或错误（缓冲区溢出）
   */
  tl::expected<void, Error> push_data(const uint8_t *data,
                                      const size_t length) {
    if (!buffer.write(data, length)) {
      return tl::unexpected(
          Error{ErrorCode::BufferOverflow, "Buffer overflow"});
    }
    return try_parse_packets();
  }

  /**
   * @brief 获取写入缓冲区（零拷贝）
   *
   * 返回一段连续的可写内存区域，供 DMA 或其他零拷贝写入使用。
   * 写入后必须调用 advance_write_index() 提交。
   *
   * @return 可写入的连续内存 span
   */
  std::span<uint8_t> get_write_buffer() noexcept {
    return buffer.get_write_buffer();
  }

  /**
   * @brief 提交写入缓冲区的数据
   *
   * 在使用 get_write_buffer() 获取的缓冲区写入后调用此方法。
   * 提交后会尝试解析新数据。
   *
   * @param length 已写入的字节数
   * @return void 或错误（提交长度无效）
   */
  tl::expected<void, Error> advance_write_index(size_t length) {
    if (!buffer.advance_write_index(length)) {
      return tl::unexpected(
          Error{ErrorCode::BufferOverflow, "Invalid advance length"});
    }
    return try_parse_packets();
  }

  /**
   * @brief 获取反序列化器的引用
   * @return 反序列化器引用
   */
  DeserializerType &get_deserializer() noexcept { return deserializer; }
  
  /**
   * @brief 获取可用数据字节数
   * @return 当前缓冲区中可读数据的总字节数
   */
  size_t available_data() const noexcept { return buffer.available(); }
  
  /**
   * @brief 获取可用写入空间
   * @return 总空闲字节数
   */
  size_t available_space() const noexcept { return buffer.space(); }
  
  /**
   * @brief 检查缓冲区是否已满
   * @return true 如果没有可用写入空间
   */
  bool is_buffer_full() const noexcept { return buffer.full(); }
  
  /**
   * @brief 清空缓冲区
   * 丢弃所有未处理的数据
   */
  void clear_buffer() noexcept { buffer.clear(); }

  /**
   * @brief 尝试解析缓冲区中的数据包
   *
   * 从当前读取位置开始扫描，查找并解析完整的数据帧。
   * 解析成功后，已处理的数据会自动从缓冲区移除。
   *
   * @return void 或错误（解析错误）
   * @note 此方法由 push_data() 和 advance_write_index() 自动调用
   *       也可以手动调用以在特定时间点触发解析
   */
  tl::expected<void, Error> try_parse_packets() {
    size_t available_bytes = buffer.available();

    // 只要有数据就开始扫描
    while (available_bytes > 0) {
      const auto buffer_view = buffer.get_contiguous_read_buffer();
      const uint8_t *data_ptr = buffer_view.data();
      const size_t view_size = buffer_view.size();

      size_t scan_offset = 0;
      bool frame_handled = false;

      while (scan_offset < view_size) {
        uint8_t worker_idx = 0xFF;

        if constexpr (unique_start_byte != 0xFF) {
          const uint8_t *next_sb = static_cast<const uint8_t *>(
              std::memchr(data_ptr + scan_offset, unique_start_byte,
                          view_size - scan_offset));
          if (!next_sb) {
            scan_offset = view_size;
            break;
          }
          scan_offset = static_cast<size_t>(next_sb - data_ptr);
          worker_idx = header_lut[unique_start_byte];
        } else {
          // 优化多起始字节扫描
          while (scan_offset < view_size &&
                 (worker_idx = header_lut[data_ptr[scan_offset]]) == 0xFF) {
            scan_offset++;
          }
          if (scan_offset >= view_size)
            break;
        }

        // 找到潜在帧头，丢弃之前的垃圾数据
        if (scan_offset > 0) {
          buffer.discard(scan_offset);
          available_bytes -= scan_offset;
        }

        ParseResult result = ParseResult::Incomplete;

        // 使用 tuple_switch 动态分发到编译期生成的 Worker
        Details::runtime_get(worker_idx, WorkerTuple{},
                             [&](auto worker_instance) {
                               using WorkerType = decltype(worker_instance);
                               result = this->parse_frame_impl<WorkerType>();
                             });

        if (result == ParseResult::Success) {
          monitor_.on_packet_received();
          available_bytes = buffer.available();
          frame_handled = true;
          break;
        } else if (result == ParseResult::Failure) {
          // 失败，丢弃起始字节，继续扫描
          buffer.discard(1);
          available_bytes--;
          frame_handled = true;
          break;
        } else {
          // Incomplete -> 等待更多数据
          return {};
        }
      }

      if (!frame_handled) {
        if (scan_offset == view_size) {
          buffer.discard(view_size);
          available_bytes -= view_size;
        }
        if (available_bytes == 0)
          break;
      }
    }
    return {};
  }

private:
  // --- 通用帧解析实现 ---
  template <typename Worker> ParseResult parse_frame_impl() {
    using P = typename Worker::Protocol;

    if (buffer.available() < P::header_size)
      return ParseResult::Incomplete;

    // 获取帧头指针，尽量避免拷贝
    uint8_t header_stack_copy[P::header_size];
    const uint8_t *header_ptr = nullptr;
    auto [hs1, hs2] = buffer.get_read_spans(0, P::header_size);
    if (hs2.empty()) {
      header_ptr = hs1.data();
    } else {
      buffer.peek(header_stack_copy, 0, P::header_size);
      header_ptr = header_stack_copy;
    }

    if constexpr (P::has_second_byte) {
      if (header_ptr[1] != P::second_byte)
        return ParseResult::Failure;
    }

    if constexpr (P::has_header_crc) {
      if (RPL::ProtocolCRC8::calc(header_ptr, 4) != header_ptr[4])
        return ParseResult::Failure;
    }

    size_t data_len = 0;
    uint16_t cmd_id = 0;

    if constexpr (Worker::is_fixed) {
      data_len = Worker::fixed_size;
      cmd_id = Worker::fixed_cmd;
    } else {
      if constexpr (P::length_field_bytes == 2) {
        std::memcpy(&data_len, header_ptr + P::length_offset, 2);
      } else {
        data_len = header_ptr[P::length_offset];
      }
      if constexpr (P::cmd_field_bytes == 2) {
        std::memcpy(&cmd_id, header_ptr + P::cmd_offset, 2);
      }
      if (data_len > max_frame_size - P::header_size - P::tail_size)
        return ParseResult::Failure;
    }

    size_t total_len = P::header_size + data_len + P::tail_size;
    if (buffer.available() < total_len)
      return ParseResult::Incomplete;

    // 获取分段读视图，进行分段 CRC 校验
    auto [s1, s2] = buffer.get_read_spans(0, total_len);

    size_t calc_len = total_len - P::tail_size;
    uint16_t calc_crc = 0;

    if (calc_len <= s1.size()) {
      calc_crc = P::RPL_CRC::calc(s1.data(), calc_len);
    } else {
      calc_crc = P::RPL_CRC::calc(s1.data(), s1.size());
      calc_crc = P::RPL_CRC::calc(s2.data(), calc_len - s1.size(), calc_crc);
    }

    // 验证接收到的 CRC
    uint16_t recv_crc = 0;
    if (calc_len + 2 <= s1.size()) {
      std::memcpy(&recv_crc, s1.data() + calc_len, 2);
    } else if (calc_len >= s1.size()) {
      std::memcpy(&recv_crc, s2.data() + (calc_len - s1.size()), 2);
    } else {
      // CRC 跨越了 A/B 边界
      recv_crc =
          s1.data()[calc_len] | (static_cast<uint16_t>(s2.data()[0]) << 8);
    }

    if (calc_crc != recv_crc)
      return ParseResult::Failure;

    // 反序列化 (分段拷贝)
    std::span<const uint8_t> payload_s1, payload_s2;

    if (P::header_size < s1.size()) {
      payload_s1 = s1.subspan(P::header_size);
      if (payload_s1.size() >= data_len) {
        payload_s1 = payload_s1.subspan(0, data_len);
        // payload_s2 为空
      } else {
        payload_s2 = s2.subspan(0, data_len - payload_s1.size());
      }
    } else {
      // Payload 完全在 s2 中
      payload_s2 = s2.subspan(P::header_size - s1.size(), data_len);
    }

    deserializer.write_segmented(cmd_id, payload_s1, payload_s2);

    // 统一丢弃
    buffer.discard(total_len);
    return ParseResult::Success;
  }
};

} // namespace RPL

#endif // RPL_PARSER_HPP
