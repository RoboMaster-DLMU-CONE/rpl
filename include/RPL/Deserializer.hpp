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
#include "Meta/PacketInfoCollector.hpp"

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

public:
  /**
   * @brief 获取指定类型的数据包
   *
   * 从内存池中获取指定类型的反序列化数据包
   *
   * @tparam T 要获取的数据包类型
   * @return 指定类型的反序列化数据包
   */
  template <typename T>
    requires Deserializable<T, Ts...>
  T get() const noexcept {
    auto ptr = reinterpret_cast<uint8_t *>(
        &pool.buffer[Collector::template type_index<T>()]);
    Meta::PacketTraits<T>::before_get(ptr);
    return *reinterpret_cast<T *>(ptr);
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
  constexpr T &getRawRef() const noexcept {
    return reinterpret_cast<T &>(
        pool.buffer[Collector::template type_index<T>()]);
  };

  /**
   * @brief 获取指定命令码的写入指针
   *
   * 获取内存池中指定命令码对应数据的写入指针，用于Parser写入数据
   *
   * @warning 存在竞态访问可能
   * @param cmd 命令码
   * @return 指向数据缓冲区的指针，如果命令码无效则返回nullptr
   */
  [[nodiscard]] constexpr uint8_t *getWritePtr(uint16_t cmd) const noexcept {
    return reinterpret_cast<uint8_t *>(&pool.buffer[Collector::cmd_index(cmd)]);
  }
};
} // namespace RPL

#endif // RPL_DESERIALIZER_HPP
