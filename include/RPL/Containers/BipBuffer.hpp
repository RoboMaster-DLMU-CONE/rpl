/**
 * @file BipBuffer.hpp
 * @brief RPL 双区缓冲区实现
 *
 * 实现一个双区环形缓冲区 (BipBuffer)，保证连续内存访问。
 * 它维护两个区域 (A 和 B)，通过在缓冲区起始处开启新区域来处理
 * 回绕 (wrap-around) 情况，避免数据分裂。
 *
 * BipBuffer 是一种特殊的环形缓冲区，它保证任何可读数据块在物理内存中
 * 是连续的。这使得解析器可以直接读取数据而无需处理回绕情况。
 *
 * @par 设计原理
 * - 区域 A: FIFO 的头部（最先读取的数据）
 * - 区域 B: FIFO 的尾部（在 A 之后读取的数据），起始位置始终为 0
 * - 写入时自动选择合适区域，避免数据跨越缓冲区边界
 *
 * @par 使用场景
 * - 流式数据包解析（如 Parser 类）
 * - DMA 直接内存访问的零拷贝写入
 * - 需要连续内存块的协议解析
 *
 * @code
 * RPL::Containers::BipBuffer<1024> buffer;
 *
 * // 获取写入缓冲区
 * auto write_span = buffer.get_write_buffer();
 * // DMA 或手动写入数据...
 * buffer.advance_write_index(received_bytes);
 *
 * // 读取数据
 * auto read_span = buffer.get_contiguous_read_buffer();
 * // 处理数据...
 * buffer.discard(processed_bytes);
 * @endcode
 *
 * @author WindWeaver
 * @date 2024
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
   *
   * @return 可写入的连续内存 span，如果缓冲区已满则返回空 span
   * @note 此方法支持零拷贝 DMA 写入
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
   * 在数据写入缓冲区后调用此方法，以提交已写入的字节数。
   * 此方法会根据当前写入区域（A 或 B）更新相应的状态。
   *
   * @param length 已写入的字节数
   * @return true 如果成功提交
   * @return false 如果提交长度超出可用空间
   * @note 此方法应与 get_write_buffer() 或 get_write_buffer_force_wrap() 配合使用
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
   *
   * @return 缓冲区起始处的 Span (区域 B)，如果无法切换则返回空 span
   * @note 此方法用于处理特殊情况，通常应使用 get_write_buffer()
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
   *
   * 将数据拷贝到内部缓冲区，自动选择合适的写入区域。
   * 如果当前区域空间不足，会尝试切换到另一区域。
   *
   * @param data 指向源数据的指针
   * @param length 要写入的字节数
   * @return true 如果成功写入
   * @return false 如果没有足够的连续空间
   * @note 此方法会执行内存拷贝，对于零拷贝场景请使用 get_write_buffer()
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
   *
   * 返回当前可用的第一个连续数据块（区域 A 的内容）。
   * 读取并处理完数据后，应调用 discard() 来释放空间。
   *
   * @return 可读的连续内存 span，如果没有数据则返回空 span
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
   *
   * 从缓冲区中移除已处理的数据，释放空间供后续写入使用。
   * 支持跨区域丢弃（同时丢弃区域 A 和 B 的数据）。
   *
   * @param length 要丢弃的字节数
   * @return true 如果成功丢弃
   * @return false 如果请求长度超过可用数据量
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
   * 返回指定偏移量和长度的数据视图，可能分为两个连续的 span。
   * 如果数据是连续的，第二个 span 为空。
   *
   * @param offset 相对于可用数据的偏移量
   * @param length 要读取的长度
   * @return std::pair 包含两个 span。如果数据是连续的，第二个 span 为空。
   *         如果请求超出范围，两个 span 都为空。
   * @note 此方法用于零拷贝 CRC 校验等场景
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

  // --- 便捷方法 ---
  
  /**
   * @brief 获取可用数据字节数
   * @return 当前缓冲区中可读数据的总字节数
   */
  size_t available() const { return region_a_size + region_b_size; }

  /**
   * @brief 获取可用写入空间
   *
   * 注意：这是总空闲字节数，不一定是连续的。
   * 实际可写入的连续空间可能小于此值。
   *
   * @return 总空闲字节数
   */
  size_t space() const {
    if (region_b_size > 0)
      return region_a_start - region_b_size;
    // 最大连续可写? 还是总空闲字节?
    // 总空闲字节:
    return (SIZE - (region_a_start + region_a_size)) + region_a_start;
  }

  /**
   * @brief 检查缓冲区是否已满
   * @return true 如果没有可用写入空间
   */
  bool full() const { return space() == 0; }
  
  /**
   * @brief 检查缓冲区是否为空
   * @return true 如果没有可读数据
   */
  bool empty() const { return available() == 0; }
  
  /**
   * @brief 清空缓冲区
   * 重置所有状态，丢弃所有数据
   */
  void clear() {
    region_a_start = region_a_size = 0;
    region_b_size = 0;
  }

  /**
   * @brief 获取缓冲区总容量
   * @return 缓冲区的总大小（编译期常量）
   */
  static constexpr size_t size() { return SIZE; }

  /**
   * @brief 窥视缓冲区数据（不丢弃）
   *
   * 将数据从缓冲区复制到外部缓冲区，但不修改读取索引。
   * 如果需要读取并丢弃数据，应使用 read() 方法。
   *
   * @param data 目标缓冲区指针
   * @param offset 相对于可用数据的偏移量
   * @param length 要读取的字节数
   * @return true 如果成功读取
   * @return false 如果请求超出可用范围
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

  /**
   * @brief 读取并丢弃数据
   *
   * 将数据从缓冲区复制到外部缓冲区，并丢弃已读取的数据。
   * 相当于 peek() 后调用 discard()。
   *
   * @param data 目标缓冲区指针
   * @param length 要读取的字节数
   * @return true 如果成功读取并丢弃
   * @return false 如果请求超出可用范围
   */
  bool read(uint8_t *data, size_t length) {
    if (!peek(data, 0, length))
      return false;
    discard(length);
    return true;
  }
};

} // namespace RPL::Containers

#endif // RPL_BIPBUFFER_HPP
