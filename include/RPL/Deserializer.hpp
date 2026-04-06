/**
 * @file Deserializer.hpp
 * @brief RPL库的反序列化器实现
 *
 * 此文件包含Deserializer类的定义，该类用于从字节数组中反序列化数据包结构。
 * 使用内存池来存储反序列化的数据包。
 *
 * @author WindWeaver
 */

#ifndef RPL_DESERIALIZER_HPP
#define RPL_DESERIALIZER_HPP

#include "Containers/MemoryPool.hpp"
#include "Meta/BitstreamParser.hpp"
#include "Meta/PacketInfoCollector.hpp"
#include "Utils/CompilerBarrier.hpp"
#ifdef RPL_USE_STD_ATOMIC
#include <atomic>
#endif
#include <cstring>
#include <span>

namespace RPL {
/**
 * @brief 可反序列化概念
 *
 * 用于检查类型T是否为可反序列化类型之一
 *
 * @tparam T 要检查的类型
 * @tparam Ts 可反序列化类型列表
 */
template <typename T, typename... Ts>
concept Deserializable = (std::is_same_v<T, Ts> || ...);

/**
 * @brief 反序列化器类
 *
 * 用于从字节数组中反序列化数据包结构，使用内存池来存储反序列化的数据
 *
 * @tparam Ts 可反序列化的数据包类型列表
 */
template <typename... Ts> class Deserializer {
  using Collector = Meta::PacketInfoCollector<Ts...>; ///< 用于收集包信息的类型
  Containers::MemoryPool<Collector> pool{}; ///< 存储反序列化数据的内存池

#ifdef RPL_USE_STD_ATOMIC
  std::atomic<uint32_t> versions_[sizeof...(Ts)]{}; ///< SeqLock version counters
#else
  volatile uint32_t versions_[sizeof...(Ts)]{}; ///< SeqLock version counters
#endif

public:
  /**
   * @brief SeqLock 写入方法
   *
   * 供 Parser 调用，写入前后递增 version（odd=writing, even=done）
   *
   * @param cmd 命令码
   * @param src 数据源指针
   * @param len 数据长度
   */
  void write(uint16_t cmd, const uint8_t *src, size_t len) noexcept {
    const auto byte_offset = Collector::cmd_index(cmd);
    if (byte_offset == static_cast<size_t>(-1))
      return;
    const auto seq_idx = Collector::cmd_seq_index(cmd);

#ifdef RPL_USE_STD_ATOMIC
    versions_[seq_idx].fetch_add(1, std::memory_order_release);
#else
    versions_[seq_idx] = versions_[seq_idx] + 1;
    compiler_barrier();
#endif

    std::memcpy(reinterpret_cast<uint8_t *>(&pool.buffer[byte_offset]), src,
                len);

#ifdef RPL_USE_STD_ATOMIC
    versions_[seq_idx].fetch_add(1, std::memory_order_release);
#else
    compiler_barrier();
    versions_[seq_idx] = versions_[seq_idx] + 1;
#endif
  }

  /**
   * @brief 分段 SeqLock 写入方法
   *
   * 用于处理跨越 BipBuffer 边界的数据包，避免中间拷贝。
   *
   * @param cmd 命令码
   * @param s1 第一段数据
   * @param s2 第二段数据
   */
  void write_segmented(uint16_t cmd, std::span<const uint8_t> s1,
                       std::span<const uint8_t> s2) noexcept {
    const auto byte_offset = Collector::cmd_index(cmd);
    if (byte_offset == static_cast<size_t>(-1))
      return;
    const auto seq_idx = Collector::cmd_seq_index(cmd);

#ifdef RPL_USE_STD_ATOMIC
    versions_[seq_idx].fetch_add(1, std::memory_order_release);
#else
    versions_[seq_idx] = versions_[seq_idx] + 1;
    compiler_barrier();
#endif

    uint8_t *dest = reinterpret_cast<uint8_t *>(&pool.buffer[byte_offset]);
    if (!s1.empty()) {
      std::memcpy(dest, s1.data(), s1.size());
    }
    if (!s2.empty()) {
      std::memcpy(dest + s1.size(), s2.data(), s2.size());
    }

#ifdef RPL_USE_STD_ATOMIC
    versions_[seq_idx].fetch_add(1, std::memory_order_release);
#else
    compiler_barrier();
    versions_[seq_idx] = versions_[seq_idx] + 1;
#endif
  }

  /**
   * @brief 获取指定类型的数据包（SeqLock 读循环）
   *
   * 从内存池中获取指定类型的反序列化数据包，
   * 通过 SeqLock 保证读取一致性
   *
   * @tparam T 要获取的数据包类型
   * @return 指定类型的反序列化数据包
   */
  template <typename T>
    requires Deserializable<T, Ts...>
  T get() noexcept {
    constexpr auto seq_idx = Collector::template type_seq_index<T>();
    T result;
    uint32_t v1, v2;
    do {
#ifdef RPL_USE_STD_ATOMIC
      v1 = versions_[seq_idx].load(std::memory_order_acquire);
#else
      v1 = versions_[seq_idx];
      compiler_barrier();
#endif

      auto ptr = reinterpret_cast<uint8_t *>(
          &pool.buffer[Collector::template type_index<T>()]);
      Meta::PacketTraits<T>::before_get(ptr);
      if constexpr (Meta::HasBitLayout<Meta::PacketTraits<T>>) {
        result = deserialize_bitstream<T>(
            std::span<const uint8_t>(ptr, Meta::PacketTraits<T>::size));
      } else {
        result = *reinterpret_cast<const T *>(ptr);
      }

#ifdef RPL_USE_STD_ATOMIC
      std::atomic_thread_fence(std::memory_order_acquire);
      v2 = versions_[seq_idx].load(std::memory_order_relaxed);
#else
      compiler_barrier();
      v2 = versions_[seq_idx];
#endif
    } while (v1 != v2 || (v1 & 1));
    return result;
  };

  /**
   * @brief 获取指定类型的直接引用
   *
   * 获取内存池中指定类型的直接引用
   *
   * @warning 存在竞态访问可能
   * @tparam T 要获取引用的数据包类型
   * @return 指定类型的直接引用
   */
  template <typename T>
    requires Deserializable<T, Ts...>
  constexpr T &getRawRef() noexcept {
    return reinterpret_cast<T &>(
        pool.buffer[Collector::template type_index<T>()]);
  };

  /**
   * @brief 获取指定命令码的写入指针
   *
   * @deprecated 请改用 write() 方法以获得 SeqLock 线程安全保护
   * @param cmd 命令码
   * @return 指向数据缓冲区的指针，如果命令码无效则返回nullptr
   */
  [[deprecated("Use write() for SeqLock-protected writes")]]
  [[nodiscard]] constexpr uint8_t *getWritePtr(uint16_t cmd) noexcept {
    const auto index = Collector::cmd_index(cmd);
    if (index == static_cast<size_t>(-1))
      return nullptr;
    return reinterpret_cast<uint8_t *>(&pool.buffer[index]);
  }
};
} // namespace RPL

#endif // RPL_DESERIALIZER_HPP
