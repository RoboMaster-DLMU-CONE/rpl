/**
 * @file MemoryPool.hpp
 * @brief RPL库的静态内存池实现
 *
 * 此文件包含 MemoryPool 类的定义，用于管理反序列化数据的内存分配。
 * 内存池通过预分配固定大小的内存块来避免动态分配，提高性能。
 *
 * MemoryPool 是一个简单的静态内存分配器，它在编译期根据 Collector
 * 提供的 totalSize 常量预分配内存。这种设计避免了运行时动态内存分配，
 * 非常适合嵌入式系统。
 *
 * @par 使用场景
 * - Deserializer 类用于存储反序列化后的数据包
 * - 需要固定内存地址且无动态分配的场景
 *
 * @author WindWeaver
 */

#ifndef RPL_MEMORYPOOL_HPP
#define RPL_MEMORYPOOL_HPP

#include <array>
#include <cstddef>

namespace RPL::Containers
{
    /**
     * @brief 静态内存池结构体
     *
     * 用于管理反序列化数据的内存分配，通过预分配固定大小的内存块来避免动态分配。
     * 内存池的大小在编译期由 Collector::totalSize 确定。
     *
     * @tparam Collector 用于收集包信息的类型，必须提供 totalSize 静态常量
     *
     * @note 内存缓冲区使用 std::max_align_t 对齐，以确保任何数据类型的访问都是安全的
     */
    template <typename Collector>
    struct MemoryPool
    {
        /**
         * @brief 预分配的内存缓冲区
         *
         * 使用 std::max_align_t 对齐的静态内存缓冲区，
         * 大小由 Collector::totalSize 在编译期确定。
         */
        alignas(std::max_align_t) std::array<std::byte, Collector::totalSize> buffer{};  ///< 预分配的内存缓冲区
    };
}

#endif //RPL_MEMORYPOOL_HPP
