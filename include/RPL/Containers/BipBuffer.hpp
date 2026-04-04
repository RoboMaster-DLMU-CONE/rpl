/**
 * @file BipBuffer.hpp
 * @brief RPL 双区缓冲区实现
 *
 * 实现一个双区环形缓冲区 (BipBuffer)，保证连续内存访问。
 * 它维护两个区域 (A 和 B)，通过在缓冲区起始处开启新区域来处理
 * 回绕 (wrap-around) 情况，避免数据分裂。
 */

#ifndef RPL_BIPBUFFER_HPP
#define RPL_BIPBUFFER_HPP

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>

namespace RPL::Containers {

/**
 * @brief 双区缓冲区类
 *
 * 保证任何可读数据块在物理内存中是连续的。
 * 消除了解析时对回绕检查的需要。
 *
 * @tparam SIZE 缓冲区大小，必须是 2 的幂
 */
template <size_t SIZE> class BipBuffer {
  static_assert((SIZE & (SIZE - 1)) == 0, "SIZE must be a power of 2");

  alignas(64) uint8_t buffer[SIZE]{};

  // 区域 A: FIFO 的头部 (最先读取的数据)
  size_t region_a_start{0};
  size_t region_a_size{0};

  // 区域 B: FIFO 的尾部 (在 A 之后读取的数据)
  // 在此实现中，如果存在区域 B，其起始位置始终为 0
  size_t region_b_size{0};

public:
  /**
   * @brief 获取连续缓冲区用于写入
   *
   * 返回可用于写入的最大连续块。
   * 如果尾部空间不足，这可能会切换到缓冲区起始处。
   */
  std::span<uint8_t> get_write_buffer() noexcept {
    // 情况 1: 区域 B 中有数据。只能追加到 B。
    // B 始终从 0 开始并增长。空间受限于 A 的起始位置。
    if (region_b_size > 0) {
      size_t free_space = region_a_start - region_b_size;
      return {buffer + region_b_size, free_space};
    }

    // 情况 2: 仅存在区域 A (或为空)。
    // 可以在 A 之后写入直到缓冲区末尾。
    // 或者如果 A 存在且 A 不在 0 位置，可以在 0 处开启 B。

    // 2a: 尝试追加到 A
    size_t a_end = region_a_start + region_a_size;
    size_t space_at_end = SIZE - a_end;

    // 如果尾部有足够空间，或者还不能切换到 B (A 从 0 开始)，返回尾部空间。

    if (space_at_end > 0) {
      return {buffer + a_end, space_at_end};
    }

    // 2b: 尾部无空间。尝试在 0 处开启区域 B。
    // 可用空间最多到 region_a_start。
    if (region_a_start > 0) {
      return {buffer, region_a_start};
    }

    // 缓冲区已满
    return {};

    // TODO: 接收模板参数，判断是否足够容纳数据包
  }

  /**
   * @brief 预留空间/提交写入
   *
   * @param length 已写入的字节数
   * @return true 如果成功
   */
  bool advance_write_index(size_t length) {
    if (length == 0)
      return true;

    // 根据状态确定写入位置
    if (region_b_size > 0) {
      // 必定写入到 B
      if (region_b_size + length > region_a_start)
        return false; // 溢出
      region_b_size += length;
      return true;
    }

    size_t a_end = region_a_start + region_a_size;

    if (length <= SIZE - a_end) {
      region_a_size += length;
      return true;
    }

    if (region_a_size == 0) {
      // 空缓冲区逻辑
      region_a_size += length;
      return true;
    }

    // 如果尾部有空间，get_write_buffer 返回尾部。
    if (SIZE - a_end > 0) {
      if (length > SIZE - a_end)
        return false;
      region_a_size += length;
      return true;
    }

    // 如果尾部无空间，get_write_buffer 返回起始处。
    // 我们创建 B。
    if (length > region_a_start)
      return false;
    region_b_size = length;
    return true;
  }

  /**
   * @brief 手动切换到区域 B (从头部写入)
   *
   * 强制缓冲区跳过尾部剩余空间，从 0 位置开始写入。
   * 当尾部剩余空间太小无法容纳 incoming 数据包时很有用。
   * @return 缓冲区起始处的 Span (区域 B)
   */
  std::span<uint8_t> get_write_buffer_force_wrap() {
    if (region_b_size > 0) {
      // 已经在 B 模式，返回 B 写入空间
      return get_write_buffer();
    }
    // 强制创建 B (如果可能)
    if (region_a_start > 0) {
      return {buffer, region_a_start};
    }
    return {};
  }

  /**
   * @brief 提交写入到强制回绕区域
   */
  bool advance_write_index_wrapped(size_t length) {
    if (region_b_size > 0) {
      return advance_write_index(length);
    }
    // 创建 B
    if (length > region_a_start)
      return false;
    region_b_size = length;
    return true;
  }

  /**
   * @brief 复制数据到缓冲区
   */
  bool write(const uint8_t *data, size_t length) {
    if (length == 0)
      return true;

    auto span = get_write_buffer();

    // 逻辑: 尝试适配当前写入区域 (A 或 B)
    if (span.size() >= length) {
      std::memcpy(span.data(), data, length);
      return advance_write_index(length);
    }

    // 如果不行，且我们正在写入 A (B 为空)，
    // 检查是否可以切换到 B。
    if (region_b_size == 0) {
      // 检查起始处空间
      if (region_a_start >= length) {
        // 切换到 B!
        std::memcpy(buffer, data, length);
        region_b_size = length;
        return true;
      }
    }

    return false; // 无足够连续空间
  }

  /**
   * @brief 获取连续数据用于读取
   */
  [[nodiscard]] std::span<const uint8_t>
  get_contiguous_read_buffer() const noexcept {
    if (region_a_size > 0) {
      return {buffer + region_a_start, region_a_size};
    }
    return {};
  }

  /**
   * @brief 丢弃数据 (推进读取索引)
   * 支持跨区域丢弃
   */
  bool discard(size_t length) {
    if (length == 0)
      return true;
    if (length > available())
      return false;

    if (length <= region_a_size) {
      region_a_start += length;
      region_a_size -= length;
    } else {
      // 跨越到区域 B
      size_t remaining_from_b = length - region_a_size;
      region_a_start = 0;
      region_a_size = 0; // 临时清空 A，触发下面的 B 提升
      region_b_size -= remaining_from_b;
      // 将 B 提升为 A，起始位置移动到 B 被扣除后的新起点
      // 注意：B 始终从 0 开始增长。扣除 B 的头部后，新的 A 应该从
      // remaining_from_b 开始。
      region_a_start = remaining_from_b;
      region_a_size = region_b_size;
      region_b_size = 0;
    }

    // 如果 A 为空且 B 为空，重置到 0
    if (region_a_size == 0) {
      if (region_b_size > 0) {
        region_a_start = 0;
        region_a_size = region_b_size;
        region_b_size = 0;
      } else {
        region_a_start = 0;
      }
    }
    return true;
  }

  /**
   * @brief 获取两个连续的读视图 (用于零拷贝分段处理)
   *
   * @param offset 相对于 available 数据的偏移量
   * @param length 长度
   * @return std::pair 包含两个 span。如果数据是连续的，第二个 span 为空。
   */
  [[nodiscard]] std::pair<std::span<const uint8_t>, std::span<const uint8_t>>
  get_read_spans(size_t offset, size_t length) const noexcept {
    if (offset + length > available())
      return {{}, {}};

    if (offset < region_a_size) {
      size_t a_readable = region_a_size - offset;
      if (length <= a_readable) {
        // 全在 A 中
        return {{buffer + region_a_start + offset, length}, {}};
      } else {
        // 跨越 A 和 B
        return {{buffer + region_a_start + offset, a_readable},
                {buffer, length - a_readable}};
      }
    } else {
      // 全在 B 中
      return {{buffer + (offset - region_a_size), length}, {}};
    }
  }

  // 便捷方法
  size_t available() const { return region_a_size + region_b_size; }

  // 空间计算很棘手:
  // 它是 max(A 尾部空间, A 起始前空间) - 但逻辑取决于状态。大约:
  size_t space() const {
    if (region_b_size > 0)
      return region_a_start - region_b_size;
    // 最大连续可写? 还是总空闲字节?
    // 总空闲字节:
    return (SIZE - (region_a_start + region_a_size)) + region_a_start;
  }

  bool full() const { return space() == 0; }
  bool empty() const { return available() == 0; }
  void clear() {
    region_a_start = region_a_size = 0;
    region_b_size = 0;
  }

  static constexpr size_t size() { return SIZE; }

  /**
   * @brief 窥视 (必要时复制，但 BipBuffer 逻辑尽量避免)
   * 注意: BipBuffer 的读取结构使得跨边界 'peek' 不复制很棘手。
   */
  bool peek(uint8_t *data, size_t offset, size_t length) const {
    if (offset + length > available())
      return false;

    // 从 A 读取
    if (offset < region_a_size) {
      size_t read_from_a = std::min(length, region_a_size - offset);
      std::memcpy(data, buffer + region_a_start + offset, read_from_a);

      if (read_from_a < length) {
        // 剩余部分从 B 读取
        std::memcpy(data + read_from_a, buffer, length - read_from_a);
      }
    } else {
      // 仅从 B 读取
      size_t offset_in_b = offset - region_a_size;
      std::memcpy(data, buffer + offset_in_b, length);
    }
    return true;
  }

  bool read(uint8_t *data, size_t length) {
    if (!peek(data, 0, length))
      return false;
    discard(length);
    return true;
  }
};

} // namespace RPL::Containers

#endif // RPL_BIPBUFFER_HPP
