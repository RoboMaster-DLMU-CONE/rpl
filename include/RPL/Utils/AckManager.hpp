#ifndef RPL_ACK_MANAGER_HPP
#define RPL_ACK_MANAGER_HPP

#include "RPL/Utils/ConnectionMonitor.hpp"
#include "RPL/Utils/Error.hpp"
#include <array>
#include <cstdint>
#include <tl/expected.hpp>

namespace RPL {

template <TickProviderConcept TickProvider>
class AckManager {
public:
  using tick_type = typename TickProvider::tick_type;

  AckManager() {
    for (auto &p : pending_)
      p.req_id = 0xFF;
  }

  uint8_t allocate() {
    uint8_t id = next_id_++;
    pending_[id].req_id = id;
    pending_[id].status = 0xFF;
    pending_[id].send_time = TickProvider::now();
    pending_[id].resolved = false;
    return id;
  }

  tl::expected<uint8_t, Error>
  wait_ack(uint8_t req_id, tick_type timeout_ms) {
    tick_type start = TickProvider::now();

    while ((TickProvider::now() - start) < timeout_ms) {
      if (pending_[req_id].resolved) {
        pending_[req_id].req_id = 0xFF;
        return pending_[req_id].status;
      }
    }

    pending_[req_id].req_id = 0xFF;
    return tl::unexpected(
        Error{ErrorCode::Timeout, "Ack timeout for req_id"});
  }

  tl::expected<uint8_t, Error>
  try_ack(uint8_t req_id) {
    if (pending_[req_id].resolved) {
      pending_[req_id].req_id = 0xFF;
      return pending_[req_id].status;
    }
    return tl::unexpected(
        Error{ErrorCode::Again, "Ack not yet received"});
  }

  void resolve(uint8_t req_id, uint8_t status) {
    if (pending_[req_id].req_id == req_id) {
      pending_[req_id].status = status;
      pending_[req_id].resolved = true;
    }
  }

  void check_timeouts(tick_type timeout_ms) {
    tick_type now = TickProvider::now();
    for (auto &p : pending_) {
      if (p.req_id != 0xFF && !p.resolved) {
        if ((now - p.send_time) >= timeout_ms) {
          p.status = 0xFF;
          p.resolved = true;
        }
      }
    }
  }

  void cancel(uint8_t req_id) {
    pending_[req_id].req_id = 0xFF;
  }

  bool is_pending(uint8_t req_id) const {
    return pending_[req_id].req_id != 0xFF && !pending_[req_id].resolved;
  }

  size_t pending_count() const {
    size_t count = 0;
    for (const auto &p : pending_)
      if (p.req_id != 0xFF && !p.resolved)
        count++;
    return count;
  }

private:
  struct PendingRequest {
    uint8_t req_id;
    uint8_t status;
    tick_type send_time;
    bool resolved;
  };
  std::array<PendingRequest, 256> pending_{};
  uint8_t next_id_{0};
};

} // namespace RPL

#endif // RPL_ACK_MANAGER_HPP
