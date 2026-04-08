/**
 * @file CompilerBarrier.hpp
 * @brief RPL 编译器屏障实现
 *
 * 此文件提供编译器屏障（Compiler Barrier）功能，
 * 用于阻止编译器对内存访问指令进行重排序优化。
 *
 * @par 设计原理
 * - 编译器屏障是一种内存屏障的轻量级形式
 * - 它告诉编译器不要跨越屏障重排内存访问
 * - 与硬件内存屏障不同，它不生成任何CPU指令
 *
 * @par 使用场景
 * - SeqLock 实现中的读写同步
 * - 无锁数据结构
 * - 禁用异常环境中的内存 ordering
 *
 * @author WindWeaver
 */

#ifndef RPL_COMPILER_BARRIER_HPP
#define RPL_COMPILER_BARRIER_HPP

namespace RPL {

/**
 * @brief 编译器屏障函数
 *
 * 阻止编译器对跨越此屏障的内存访问进行重排序优化。
 * 这是一个轻量级的软件屏障，不生成任何 CPU 指令。
 *
 * @note 在 GCC/Clang 上使用 inline asm，在 MSVC 上使用 _ReadWriteBarrier
 * @warning 此函数仅提供编译器级别的屏障，不提供硬件级别的内存排序保证
 *          在需要更强保证的场景中，应使用 std::atomic 或硬件屏障
 */
#ifndef compiler_barrier
inline void compiler_barrier() noexcept {
#if defined(_MSC_VER)
  _ReadWriteBarrier();
#else
  asm volatile("" ::: "memory");
#endif
}
#endif

} // namespace RPL

#endif // RPL_COMPILER_BARRIER_HPP
