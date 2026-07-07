#ifndef RPL_USB_TRANSPORT_HPP
#define RPL_USB_TRANSPORT_HPP

#include "RPL/Deserializer.hpp"
#include "RPL/Parser.hpp"
#include "RPL/Serializer.hpp"
#include "RPL/Utils/AckManager.hpp"
#include <cstdint>
#include <tl/expected.hpp>

namespace RPL {
namespace detail {
struct NoSend {
  void operator()(const uint8_t *, size_t) const {}
  explicit operator bool() const { return false; }
};
} // namespace detail

template <typename AckMgr, typename SendFunc, typename... Packets>
class USBTransport {
public:
  using tick_type = typename AckMgr::tick_type;

  USBTransport() : parser_(deserializer_) {}

  explicit USBTransport(SendFunc cb)
      : parser_(deserializer_), send_cb_(std::move(cb)), has_send_(true) {}

  Deserializer<Packets...> &deserializer() noexcept { return deserializer_; }
  const Deserializer<Packets...> &deserializer() const noexcept {
    return deserializer_;
  }

  Parser<Packets...> &parser() noexcept { return parser_; }
  const Parser<Packets...> &parser() const noexcept { return parser_; }

  AckMgr &ack_manager() noexcept { return ack_mgr_; }
  const AckMgr &ack_manager() const noexcept { return ack_mgr_; }

  void on_send(SendFunc cb) {
    send_cb_ = std::move(cb);
    has_send_ = true;
  }

  bool has_send() const { return has_send_; }

  tl::expected<void, Error> receive(const uint8_t *buf, size_t len) {
    if (len >= 4) {
      uint8_t cmd = buf[2];
      if (cmd == Meta::PacketTraits<USBAck>::cmd && len >= 5) {
        uint8_t req_id = buf[3];
        uint8_t status = buf[4];
        ack_mgr_.resolve(req_id, status);
      }
    }
    return parser_.push_data(buf, len);
  }

  template <typename T>
    requires Serializable<T, Packets...>
  tl::expected<void, Error> notify(const T &packet) {
    if (!has_send_)
      return tl::unexpected(
          Error{ErrorCode::InternalError, "Send callback not set"});

    constexpr size_t frame_sz =
        Serializer<Packets...>::template frame_size<T>();
    uint8_t buffer[frame_sz];

    auto result = serializer_.serialize(buffer, frame_sz, packet);
    if (!result)
      return tl::unexpected(result.error());

    send_cb_(buffer, *result);
    return {};
  }

  template <typename T>
    requires Serializable<T, Packets...>
  tl::expected<uint8_t, Error> request(T &packet, tick_type timeout_ms) {
    if (!has_send_)
      return tl::unexpected(
          Error{ErrorCode::InternalError, "Send callback not set"});

    if constexpr (Meta::PacketTraits<T>::category ==
                  Meta::PacketCategory::Request) {
      packet.req_id = ack_mgr_.allocate();
    }

    constexpr size_t frame_sz =
        Serializer<Packets...>::template frame_size<T>();
    uint8_t buffer[frame_sz];

    auto result = serializer_.serialize(buffer, frame_sz, packet);
    if (!result)
      return tl::unexpected(result.error());

    send_cb_(buffer, *result);
    return ack_mgr_.wait_ack(packet.req_id, timeout_ms);
  }

  void poll() { ack_mgr_.check_timeouts(default_timeout_ms_); }

  void set_default_timeout(tick_type timeout_ms) {
    default_timeout_ms_ = timeout_ms;
  }

private:
  Deserializer<Packets...> deserializer_{};
  Serializer<Packets...> serializer_{};
  Parser<Packets...> parser_;
  [[no_unique_address]] AckMgr ack_mgr_{};
  [[no_unique_address]] SendFunc send_cb_{};
  bool has_send_{false};
  tick_type default_timeout_ms_{100};
};

template <typename AckMgr, typename... Packets>
using USBTransportFn =
    USBTransport<AckMgr, void (*)(const uint8_t *, size_t), Packets...>;

} // namespace RPL

#endif // RPL_USB_TRANSPORT_HPP
