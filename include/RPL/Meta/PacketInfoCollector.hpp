/**
 * @file PacketInfoCollector.hpp
 * @brief RPL库的数据包信息收集器
 *
 * 此文件定义了数据包信息收集器，用于管理数据包的元信息，
 * 包括总大小、命令码到索引的映射等。
 *
 * PacketInfoCollector 在编译期计算所有数据包的内存布局，
 * 考虑对齐要求，并生成高效的查找表（使用 frozen 库）。
 *
 * @par 设计原理
 * - 使用编译期计算避免运行时开销
 * - 考虑内存对齐以确保安全访问
 * - 使用 frozen::unordered_map 提供高效的编译期查找表
 *
 * @author WindWeaver
 */

#ifndef RPL_PACKETINFOCOLLECTOR_HPP
#define RPL_PACKETINFOCOLLECTOR_HPP
#include "PacketTraits.hpp"
#include <algorithm>
#include <array>
#include <utility>

#include <frozen/unordered_map.h>

namespace RPL::Meta {
/**
 * @brief 编译期对齐计算辅助函数
 *
 * 将 offset 向上对齐到 alignment 的倍数。
 *
 * @param offset 当前偏移量
 * @param alignment 对齐要求（必须是 2 的幂）
 * @return 对齐后的偏移量
 */
constexpr size_t align_up(size_t offset, size_t alignment) {
  return (offset + alignment - 1) & ~(alignment - 1);
}

/**
 * @brief 数据包信息收集器
 *
 * 用于收集和管理数据包的元信息，包括总大小、命令码到索引的映射等。
 * 所有计算都在编译期完成，运行时零开销。
 *
 * @tparam Ts 数据包类型列表
 *
 * @par 主要功能
 * - 计算所有数据包在内存池中的布局（考虑对齐）
 * - 生成命令码到内存偏移量的映射表
 * - 生成命令码到序列索引的映射表（用于 SeqLock）
 *
 * @code
 * using Collector = RPL::Meta::PacketInfoCollector<PacketA, PacketB>;
 * constexpr size_t total_size = Collector::totalSize;
 * constexpr size_t offset = Collector::cmd_index(0x0102);
 * @endcode
 */
template <typename... Ts> struct PacketInfoCollector {
  /**
   * @brief 递归计算偏移量的辅助函数
   *
   * 遍历所有类型，计算每个类型在内存池中的对齐后偏移量。
   *
   * @tparam T 当前处理的类型
   * @tparam Rest 剩余类型列表
   * @param offsets 存储所有偏移量的数组
   * @param current_offset 当前偏移量（会更新）
   * @param index 当前类型的索引
   */
  template <typename T, typename... Rest>
  static constexpr void calculate_offsets(
      std::array<size_t, sizeof...(Ts)> &offsets, size_t &current_offset,
      size_t index) {
    current_offset = align_up(current_offset, alignof(T));
    offsets[index] = current_offset;
    current_offset += sizeof(T);

    if constexpr (sizeof...(Rest) > 0) {
      calculate_offsets<Rest...>(offsets, current_offset, index + 1);
    }
  }

  /**
   * @brief 计算所有数据包在内存池中的布局（考虑对齐）
   *
   * 此静态常量在编译期计算所有数据包的内存布局，
   * 包括每个数据包的偏移量和总大小。
   */
  static constexpr auto layout = []() {
    struct LayoutInfo {
      std::array<size_t, sizeof...(Ts)> offsets;
      size_t total_size;
    } info{};

    size_t current_offset = 0;
    calculate_offsets<Ts...>(info.offsets, current_offset, 0);

    info.total_size = current_offset;
    return info;
  }();

  static constexpr std::size_t totalSize =
      layout.total_size; ///< 所有数据包类型的总大小（含对齐填充）

  /**
   * @brief 命令码到索引的映射
   *
   * 静态常量映射，将命令码映射到在内存池中的偏移量。
   * 使用 frozen::unordered_map 实现编译期查找表。
   */
  static constexpr auto cmdToIndex = []() {
    std::array<std::pair<uint16_t, size_t>, sizeof...(Ts)> pairs{};
    size_t index = 0;

    // 使用折叠表达式填充数组
    ((pairs[index] =
          std::make_pair(PacketTraits<Ts>::cmd, layout.offsets[index]),
      ++index),
     ...);

    return frozen::make_unordered_map(pairs);
  }();

  /**
   * @brief 获取指定类型的索引
   *
   * 获取指定数据包类型在内存池中的索引（偏移量）
   *
   * @tparam T 数据包类型
   * @return 该类型的索引（偏移量）
   */
  template <typename T> static constexpr size_t type_index() noexcept {
    return cmd_index(PacketTraits<T>::cmd);
  }

  /**
   * @brief 根据命令码获取索引
   *
   * 根据命令码获取对应数据包类型在内存池中的索引（偏移量）
   *
   * @param cmd 命令码
   * @return 对应的索引（偏移量），如果命令码不存在则返回-1
   */
  static constexpr size_t cmd_index(uint16_t cmd) noexcept {
    auto it = cmdToIndex.find(cmd);
    return it != cmdToIndex.end() ? it->second : static_cast<size_t>(-1);
  }

  /**
   * @brief 命令码到序列索引的映射（0-based 类型序号，用于 SeqLock version 数组）
   *
   * 此映射将命令码映射到其在模板参数列表中的序号（0, 1, 2, ...）。
   * 用于 SeqLock 机制中定位对应的 version 计数器。
   */
  static constexpr auto cmdToSeqIndex = []() {
    std::array<std::pair<uint16_t, size_t>, sizeof...(Ts)> pairs{};
    size_t index = 0;
    ((pairs[index] = std::make_pair(PacketTraits<Ts>::cmd, index), ++index),
     ...);
    return frozen::make_unordered_map(pairs);
  }();

  /**
   * @brief 根据命令码获取序列索引
   *
   * @param cmd 命令码
   * @return 对应的序列索引（0-based 类型序号），如果命令码不存在则返回-1
   */
  static constexpr size_t cmd_seq_index(uint16_t cmd) noexcept {
    auto it = cmdToSeqIndex.find(cmd);
    return it != cmdToSeqIndex.end() ? it->second : static_cast<size_t>(-1);
  }

  /**
   * @brief 获取指定类型的序列索引
   *
   * @tparam T 数据包类型
   * @return 该类型的序列索引（0-based 类型序号）
   */
  template <typename T> static constexpr size_t type_seq_index() noexcept {
    return cmd_seq_index(PacketTraits<T>::cmd);
  }
};
} // namespace RPL::Meta

#endif // RPL_PACKETINFOCOLLECTOR_HPP
