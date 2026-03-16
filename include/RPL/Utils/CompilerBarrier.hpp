#ifndef RPL_COMPILER_BARRIER_HPP
#define RPL_COMPILER_BARRIER_HPP

namespace RPL {
inline void compiler_barrier() noexcept {
#if defined(_MSC_VER)
  _ReadWriteBarrier();
#else
  asm volatile("" ::: "memory");
#endif
}
} // namespace RPL

#endif // RPL_COMPILER_BARRIER_HPP
