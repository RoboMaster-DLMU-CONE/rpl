#ifndef RPL_COMPILER_BARRIER_HPP
#define RPL_COMPILER_BARRIER_HPP

namespace RPL {
#ifndef compiler_barrier
#define
inline void compiler_barrier() noexcept {
#if defined(_MSC_VER)
  _ReadWriteBarrier();
#else
  asm volatile("" ::: "memory");
#endif
}
 // namespace RPL
#endif
}
#endif // RPL_COMPILER_BARRIER_HPP
