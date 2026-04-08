/**
 * @file ConnectionMonitor.hpp
 * @brief RPL 的连接健康检测工具
 *
 * 此文件提供连接监控策略类，用于检测通信链路是否正常工作。
 * 采用编译期策略模式，不需要监控时零开销。
 *
 * @par 设计原理
 * - 使用策略模式（Strategy Pattern）实现零开销抽象
 * - NullConnectionMonitor 在不需要监控时被完全优化掉
 * - TickConnectionMonitor 支持基于时间戳的超时检测
 * - CallbackConnectionMonitor 允许用户自定义回调逻辑
 *
 * @par 使用场景
 * - 检测通信链路是否断开
 * - 超时重连逻辑
 * - 自定义数据包接收事件处理
 *
 * @author WindWeaver
 */

#ifndef RPL_CONNECTION_MONITOR_HPP
#define RPL_CONNECTION_MONITOR_HPP

#include <concepts>
#include <cstdint>
#include <type_traits>

namespace RPL {

/**
 * @brief 连接监控器概念
 *
 * 定义连接监控器必须满足的接口要求。
 * 任何连接监控器类型必须实现 on_packet_received() 方法。
 *
 * @tparam T 要检查的类型
 */
template <typename T>
concept ConnectionMonitorConcept = requires(T &monitor) {
  { monitor.on_packet_received() } -> std::same_as<void>;
};

/**
 * @brief 空连接监控器 (零开销默认实现)
 *
 * 当不需要连接监控功能时使用此策略。
 * 所有方法均为空实现，编译器会将其完全优化掉。
 *
 * @par 使用示例
 * @code
 * //  Parser 默认使用此监控器
 * RPL::Parser<PacketA, PacketB> parser{deserializer};
 * // 等同于:
 * RPL::Parser<RPL::NullConnectionMonitor, PacketA, PacketB> parser{deserializer};
 * @endcode
 */
struct NullConnectionMonitor {
  /**
   * @brief 数据包接收通知 (空实现)
   *
   * 此方法为空操作，编译器会将其完全优化掉。
   */
  constexpr void on_packet_received() noexcept {}
};

static_assert(ConnectionMonitorConcept<NullConnectionMonitor>,
              "NullConnectionMonitor must satisfy ConnectionMonitorConcept");

/**
 * @brief Tick 提供器概念
 *
 * 定义时间戳提供器必须满足的接口要求。
 * Tick 提供器必须定义 tick_type 类型和 now() 静态方法。
 *
 * @tparam T 要检查的类型
 */
template <typename T>
concept TickProviderConcept = requires {
  typename T::tick_type;
  { T::now() } -> std::convertible_to<typename T::tick_type>;
};

/**
 * @brief 基于时间戳的连接监控器
 *
 * 记录最后一次成功接收数据包的时间戳，
 * 支持检测连接是否在指定超时时间内活跃。
 *
 * @tparam TickProvider 时间戳提供器类型，需满足 TickProviderConcept
 *
 * @par TickProvider 实现示例
 * @code
 * // STM32 HAL 示例
 * struct HALTickProvider {
 *     using tick_type = uint32_t;
 *     static tick_type now() { return HAL_GetTick(); }
 * };
 * 
 * // Linux 时间戳示例
 * struct LinuxTickProvider {
 *     using tick_type = uint64_t;
 *     static tick_type now() { 
 *         struct timespec ts;
 *         clock_gettime(CLOCK_MONOTONIC, &ts);
 *         return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
 *     }
 * };
 * @endcode
 *
 * @par 使用示例
 * @code
 * using Monitor = RPL::TickConnectionMonitor<HALTickProvider>;
 * RPL::Parser<Monitor, PacketA, PacketB> parser{deserializer};
 *
 * // 检查连接状态
 * if (!parser.get_connection_monitor().is_connected(100)) {
 *     // 超过 100ms 未收到数据
 * }
 * @endcode
 */
template <TickProviderConcept TickProvider> class TickConnectionMonitor {
public:
  /// @brief 时间戳类型（由 TickProvider 定义）
  using tick_type = typename TickProvider::tick_type;

  /**
   * @brief 数据包接收通知
   *
   * 由 Parser 在成功解析数据包后调用，更新最后接收时间戳。
   */
  void on_packet_received() noexcept { last_tick_ = TickProvider::now(); }

  /**
   * @brief 检查连接是否活跃
   *
   * @param timeout 超时阈值（单位由 TickProvider 决定，通常为毫秒）
   * @return true 如果在超时时间内收到过数据包
   * @return false 如果超过超时时间未收到数据包
   */
  [[nodiscard]] bool is_connected(tick_type timeout) const noexcept {
    return (TickProvider::now() - last_tick_) < timeout;
  }

  /**
   * @brief 获取最后接收时间戳
   *
   * @return 最后一次成功接收数据包的时间戳
   */
  [[nodiscard]] tick_type get_last_tick() const noexcept { return last_tick_; }

  /**
   * @brief 获取自最后接收以来经过的时间
   *
   * @return 距离最后一次成功接收数据包经过的时间（与 tick_type 同单位）
   */
  [[nodiscard]] tick_type get_elapsed() const noexcept {
    return TickProvider::now() - last_tick_;
  }

  /**
   * @brief 重置监控器状态
   *
   * 将最后接收时间戳设为当前时间，相当于重新建立连接。
   */
  void reset() noexcept { last_tick_ = TickProvider::now(); }

private:
  volatile tick_type last_tick_{};
};

// 验证 TickConnectionMonitor 满足概念要求
namespace detail {
struct MockTickProvider {
  using tick_type = uint32_t;
  static tick_type now() { return 0; }
};
static_assert(
    ConnectionMonitorConcept<TickConnectionMonitor<MockTickProvider>>,
    "TickConnectionMonitor must satisfy ConnectionMonitorConcept");
} // namespace detail

/**
 * @brief 自定义回调连接监控器
 *
 * 允许用户提供自定义的回调函数，在收到数据包时执行。
 * 回调函数在编译期指定，零运行时开销。
 *
 * @tparam Callback 静态回调函数类型（必须实现 on_packet() 静态方法）
 *
 * @par 使用示例
 * @code
 * struct MyCallback {
 *     static void on_packet() {
 *         // 自定义逻辑：计数器++、设置标志位等
 *         packet_count++;
 *     }
 *     static inline uint32_t packet_count = 0;
 * };
 *
 * using Monitor = RPL::CallbackConnectionMonitor<MyCallback>;
 * RPL::Parser<Monitor, PacketA> parser{deserializer};
 * @endcode
 */
template <typename Callback>
  requires requires { Callback::on_packet(); }
struct CallbackConnectionMonitor {
  /**
   * @brief 数据包接收通知
   *
   * 调用用户提供的回调函数。
   */
  constexpr void on_packet_received() noexcept { Callback::on_packet(); }
};

} // namespace RPL

#endif // RPL_CONNECTION_MONITOR_HPP
