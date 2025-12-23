/**
 * @file MemoryPool.hpp
 * @brief RPL库的内存池实现
 *
 * 此文件包含MemoryPool类的定义，用于管理反序列化数据的内存分配。
 * 内存池通过预分配固定大小的内存块来避免动态分配，提高性能。
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
     * @brief 内存池结构体
     *
     * 用于管理反序列化数据的内存分配，通过预分配固定大小的内存块来避免动态分配
     *
     * @tparam Collector 用于收集包信息的类型，提供totalSize常量
     */
    template <typename Collector>
    struct MemoryPool
    {
        static inline std::array<std::byte, Collector::totalSize> buffer{};  ///< 预分配的内存缓冲区
    };
}

#endif //RPL_MEMORYPOOL_HPP
