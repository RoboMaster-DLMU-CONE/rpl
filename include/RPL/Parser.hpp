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

#include "Containers/BipBuffer.hpp"
#include "Deserializer.hpp"
#include "Meta/PacketTraits.hpp"
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

namespace RPL {

namespace Details {
// --- 类型去重工具 ---
template <typename... Ts> struct TypeList {};

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

// --- 运行时获取 Tuple 元素 ---
template <typename Tuple, typename Fn, size_t... Is>
constexpr void tuple_switch(size_t i, Tuple &&t, Fn &&f,
                            std::index_sequence<Is...>) {
  ((i == Is ? f(std::get<Is>(t)) : void()), ...);
}

template <typename Tuple, typename Fn>
constexpr void runtime_get(size_t i, Tuple &&t, Fn &&f) {
  tuple_switch(i, std::forward<Tuple>(t), std::forward<Fn>(f),
               std::make_index_sequence<
                   std::tuple_size_v<std::remove_reference_t<Tuple>>>{});
}
} // namespace Details

/**
 * @brief 解析器类
 *
 * 用于解析流式数据包，支持多协议混合解析。
 * 根据 PacketTraits 中定义的 Protocol 自动生成查找表和解析逻辑。
 *
 * @tparam Ts 可解析的数据包类型列表
 */
template <typename... Ts> class Parser {
  // --- 1. 定义解析 Worker ---
  // 每个 Worker 负责处理一种特定的协议配置
  // 如果是动态命令码协议(RoboMaster)，CmdId 和 DataSize 为 0 (未使用)
  // 如果是固定协议(新遥控)，CmdId 和 DataSize 为具体值
  template <typename P, bool IsFixed, uint16_t CmdId, size_t DataSize>
  struct ProtocolWorker {
    using Protocol = P;
    static constexpr bool is_fixed = IsFixed;
    static constexpr uint16_t fixed_cmd = CmdId;
    static constexpr size_t fixed_size = DataSize;

    static constexpr size_t min_frame_size =
        P::header_size + (IsFixed ? DataSize : 0) + P::tail_size;
  };

  // --- 2. 为每个 T 提取 Worker 类型 ---
  template <typename T> struct GetWorker {
    using P = typename Meta::PacketTraits<T>::Protocol;
    static constexpr bool IsFixed = !P::has_cmd_field;
    // 如果是固定协议，必须绑定具体的 CMD 和 Size
    // 如果是动态协议，这些参数不影响 Worker 类型（归一化为 0）
    static constexpr uint16_t C = IsFixed ? Meta::PacketTraits<T>::cmd : 0;
    static constexpr size_t S = IsFixed ? Meta::PacketTraits<T>::size : 0;
    using type = ProtocolWorker<P, IsFixed, C, S>;
  };

  // --- 3. 生成去重后的 Worker 列表 ---
  // 例如：10个 RoboMaster 包 -> 1个 Worker
  //       1个 新遥控包 -> 1个 Worker
  template <typename TypeList> struct TupleFromList;
  template <typename... Ws> struct TupleFromList<Details::TypeList<Ws...>> {
    using type = std::tuple<Ws...>;
  };

  using AllWorkers = Details::TypeList<typename GetWorker<Ts>::type...>;
  using UniqueWorkers = Details::UniqueTypes_t<typename GetWorker<Ts>::type...>;
  using WorkerTuple = typename TupleFromList<UniqueWorkers>::type;

  // --- 4. 编译期计算 Max Frame Size ---
  template <typename W> static constexpr size_t get_worker_max_size() {
    if constexpr (W::is_fixed) {
      return W::min_frame_size;
    } else {
      // 动态协议，取所有使用该协议的 Packet 的最大值
      return 256 + W::Protocol::header_size + W::Protocol::tail_size;
    }
  }

  // 实际上我们需要遍历所有 Ts 来找最大值，比较稳妥
  static constexpr size_t calculate_max_frame_size() {
    size_t max = 0;
    auto check = [&max]<typename T>() {
      using P = typename Meta::PacketTraits<T>::Protocol;
      size_t size = P::header_size + Meta::PacketTraits<T>::size + P::tail_size;
      if (size > max)
        max = size;
    };
    (check.template operator()<Ts>(), ...);
    return max;
  }

  static constexpr size_t max_frame_size = calculate_max_frame_size();

  // --- 5. 计算 Buffer Size ---
  static consteval size_t calculate_buffer_size() {
    constexpr size_t min_size = max_frame_size * 4;
    if constexpr (std::has_single_bit(min_size))
      return min_size;
    else
      return std::bit_ceil(min_size);
  }
  static constexpr size_t buffer_size = calculate_buffer_size();

  // --- 成员变量 ---
  Containers::BipBuffer<buffer_size> buffer;
  std::array<uint8_t, max_frame_size> parse_buffer{};
  Deserializer<Ts...> &deserializer;

  // --- 6. 构建查找表 (Start Byte -> Worker Index) ---
  static constexpr auto header_lut = []() {
    std::array<uint8_t, 256> table;
    table.fill(0xFF); // 0xFF 表示无效

    auto register_worker = [&table]<typename W>(size_t index) {
      uint8_t sb = W::Protocol::start_byte;
      // 检查冲突：如果该字节已被占用，且不是同一个 Worker (理论上去重后 index
      // 唯一) 这里主要防备不同 Worker 使用相同 StartByte
      if (table[sb] != 0xFF && table[sb] != index) {
        // Compile-time error force
#ifndef EXCEPTION_DUMP
        throw "Header collision detected";
#else
        assert(false);
#endif
      }
      // assert(table[sb] != 0xFF && table[sb] != index);
      // TODO: Better comptime exception throwing
      table[sb] = static_cast<uint8_t>(index);
    };

    // 遍历 UniqueWorkers
    auto helper = [&register_worker]<typename... Ws>(Details::TypeList<Ws...>) {
      size_t idx = 0;
      ((register_worker.template operator()<Ws>(idx++)), ...);
    };
    helper(UniqueWorkers{});

    return table;
  }();

  // 解析结果枚举
  enum class ParseResult { Success, Failure, Incomplete };

public:
  explicit Parser(Deserializer<Ts...> &des) : deserializer(des) {}

  tl::expected<void, Error> push_data(const uint8_t *data,
                                      const size_t length) {
    if (!buffer.write(data, length)) {
      return tl::unexpected(
          Error{ErrorCode::BufferOverflow, "Buffer overflow"});
    }
    return try_parse_packets();
  }

  std::span<uint8_t> get_write_buffer() noexcept {
    return buffer.get_write_buffer();
  }

  tl::expected<void, Error> advance_write_index(size_t length) {
    if (!buffer.advance_write_index(length)) {
      return tl::unexpected(
          Error{ErrorCode::BufferOverflow, "Invalid advance length"});
    }
    return try_parse_packets();
  }

  Deserializer<Ts...> &get_deserializer() noexcept { return deserializer; }
  size_t available_data() const noexcept { return buffer.available(); }
  size_t available_space() const noexcept { return buffer.space(); }
  bool is_buffer_full() const noexcept { return buffer.full(); }
  void clear_buffer() noexcept { buffer.clear(); }

  tl::expected<void, Error> try_parse_packets() {
    size_t available_bytes = buffer.available();

    // 最小帧头长度，为了安全起见取最小的一个，或者简单的 1 (只要有 start byte
    // 就能查表) 实际解析中会检查具体 Protocol 的 header_size
    while (available_bytes > 0) {
      const auto buffer_view = buffer.get_contiguous_read_buffer();
      const uint8_t *data_ptr = buffer_view.data();
      const size_t view_size = buffer_view.size();

      size_t scan_offset = 0;
      bool frame_handled = false;

      while (scan_offset < view_size) {
        const uint8_t current_byte = data_ptr[scan_offset];
        const uint8_t worker_idx = header_lut[current_byte];

        if (worker_idx == 0xFF) {
          // 不是帧头，跳过
          scan_offset++;
          continue;
        }

        // 找到潜在帧头，尝试调用对应的 Worker
        // 需要先丢弃这一段之前的垃圾数据
        if (scan_offset > 0) {
          buffer.discard(scan_offset);
          available_bytes -= scan_offset;
          // 更新视图（虽然 loop 会 break 但为了逻辑清晰）
        }

        ParseResult result = ParseResult::Incomplete;

        // Pass the remaining view to parse_frame_impl for optimization
        auto current_view = buffer_view.subspan(scan_offset);

        // 使用 tuple_switch 动态分发到编译期生成的 Worker
        Details::runtime_get(
            worker_idx, WorkerTuple{}, [&](auto worker_instance) {
              using WorkerType = decltype(worker_instance);
              result = this->parse_frame_impl<WorkerType>(current_view);
            });

        if (result == ParseResult::Success) {
          // 成功，继续下一次大循环（重新获取 view）
          available_bytes = buffer.available();
          frame_handled = true;
          break;
        } else if (result == ParseResult::Failure) {
          // 失败，丢弃 1 字节（Start Byte），继续扫描
          buffer.discard(1);
          available_bytes--;
          // 重新获取 view 指针位置
          // 简单起见，break 出去重新 get buffer view
          // 也可以优化为 scan_offset = 0 (因为 discard 移除了数据) 但 view 变了
          frame_handled = true;
          break;
        } else {
          // Incomplete -> 等待更多数据
          return {};
        }
      }

      if (!frame_handled) {
        // 当前 view 扫描完了都没找到，或者最后剩下的一点不够
        // 如果 scan_offset == view_size，说明全是垃圾，已全部丢弃
        if (scan_offset == view_size) {
          buffer.discard(view_size);
          available_bytes -= view_size;
        }
        // 如果 available 还有数据 (Wrap around 的情况)，继续大循环
        if (available_bytes == 0)
          break;
      }
    }
    return {};
  }

private:
  // --- 通用帧解析实现 ---
  template <typename Worker>
  ParseResult parse_frame_impl(std::span<const uint8_t> view) {
    using P = typename Worker::Protocol;

    // 1. 检查数据量是否足够读取帧头
    if (buffer.available() < P::header_size)
      return ParseResult::Incomplete;

    // 2. 读取帧头
    const uint8_t *header_ptr = nullptr;
    std::array<uint8_t, P::header_size> header_copy;

    if (view.size() >= P::header_size) {
      header_ptr = view.data();
    } else {
      if (!buffer.peek(header_copy.data(), 0, P::header_size))
        return ParseResult::Incomplete;
      header_ptr = header_copy.data();
    }

    // 3. 验证 Start Byte (双重检查，虽然 LUT 已经过了)
    if (header_ptr[0] != P::start_byte)
      return ParseResult::Failure;

    // 4. 验证 Second Byte (如果有)
    if constexpr (P::has_second_byte) {
      if (header_ptr[1] != P::second_byte)
        return ParseResult::Failure;
    }

    // 5. 验证 Header CRC (如果有)
    if constexpr (P::has_header_crc) {
      if (RPL::ProtocolCRC8::calc(header_ptr, 4) != header_ptr[4])
        return ParseResult::Failure;
    }

    // 6. 获取 Payload Length 和 CMD
    size_t data_len = 0;
    uint16_t cmd_id = 0;

    // 获取长度
    if constexpr (Worker::is_fixed) {
      data_len = Worker::fixed_size;
    } else {
      if constexpr (P::length_field_bytes == 2) {
        std::memcpy(&data_len, header_ptr + P::length_offset, 2);
      } else {
        data_len = header_ptr[P::length_offset];
      }

      if (data_len > max_frame_size - P::header_size - P::tail_size)
        return ParseResult::Failure;
    }

    // 获取 CMD
    if constexpr (Worker::is_fixed) {
      cmd_id = Worker::fixed_cmd;
    } else {
      if constexpr (P::cmd_field_bytes == 2) {
        std::memcpy(&cmd_id, header_ptr + P::cmd_offset, 2);
      } else {
        // handle other sizes
      }
    }

    // 7. 检查总长度是否足够
    size_t total_len = P::header_size + data_len + P::tail_size;
    if (buffer.available() < total_len)
      return ParseResult::Incomplete;

    // --- Fast Path: 全都在 view 中 (Zero Copy) ---
    if (view.size() >= total_len) {
      const uint8_t *frame_start = view.data();

      // 8. 验证整包 CRC
      size_t calc_len = total_len - P::tail_size;
      uint16_t calc_crc = P::RPL_CRC::calc(frame_start, calc_len);

      uint16_t recv_crc = 0;
      std::memcpy(&recv_crc, frame_start + calc_len, 2);

      if (calc_crc != recv_crc)
        return ParseResult::Failure;

      // 9. 反序列化
      buffer.discard(P::header_size);

      // discard does not invalidate the view pointer if no wrap occurs,
      // but internal buffer state changes.
      // frame_start points to header start.
      // data start = frame_start + P::header_size

      deserializer.write(cmd_id, frame_start + P::header_size, data_len);

      // discard data and tail
      buffer.discard(data_len + P::tail_size);
      return ParseResult::Success;
    }

    // --- Slow Path: 跨越边界 (Split Packet) ---
    // 必须拷贝到连续内存进行 CRC 和 解析
    // 复用 parse_buffer

    if (!buffer.peek(parse_buffer.data(), 0, total_len))
      return ParseResult::Incomplete;

    // 8. 验证整包 CRC
    size_t calc_len = total_len - P::tail_size;
    uint16_t calc_crc = P::RPL_CRC::calc(parse_buffer.data(), calc_len);

    uint16_t recv_crc = 0;
    std::memcpy(&recv_crc, parse_buffer.data() + calc_len, 2);

    if (calc_crc != recv_crc)
      return ParseResult::Failure;

    // 9. 反序列化
    buffer.discard(P::header_size);

    deserializer.write(cmd_id, parse_buffer.data() + P::header_size, data_len);
    buffer.discard(data_len);

    buffer.discard(P::tail_size);
    return ParseResult::Success;
  }
};

} // namespace RPL

#endif // RPL_PARSER_HPP
