#pragma once

#include <chrono>

#include "envoy/common/time.h"

#include "common/common/lock_guard.h"
#include "common/common/thread.h"

namespace Envoy {
namespace Extensions {
namespace Tracers {
namespace XRay {

/**
 * Simple token-bucket algorithm that enables counting samples/traces used per second.
 */
class Reservoir {
public:
  /**
   * Creates a new reservoir that allows up to |traces_per_second| samples.
   */
  explicit Reservoir(uint32_t traces_per_second) : quota_(traces_per_second), used_(0) {}

  Reservoir(const Reservoir& other)
      : quota_(other.quota_), used_(other.used_), current_second_(other.current_second_) {}

  Reservoir& operator=(const Reservoir& other) {
    if (this == &other) {
      return *this;
    }
    quota_ = other.quota_;
    used_ = other.used_;
    current_second_ = other.current_second_;
    return *this;
  }

  /**
   * Determines whether all samples have been used up for this particular second.
   * Every second, this reservoir starts over with a full bucket.
   *
   * @param now Used to compare against the last recorded time to determine if it's still within the
   * same second.
   */
  bool take(Envoy::MonotonicTime now) {
    Envoy::Thread::LockGuard lg(sync_);
    const auto diff = now - current_second_;
    if (diff > std::chrono::seconds(1)) {
      used_ = 0;
      current_second_ = now;
    }

    if (used_ >= quota_) {
      return false;
    }

    ++used_;
    return true;
  }

protected:
  uint32_t quota_;
  uint32_t used_;
  Envoy::MonotonicTime current_second_;
  mutable Envoy::Thread::MutexBasicLockable sync_;
};

/**
 * Simple token-bucket algorithm that enables counting samples/traces used per second, which can
 * also be updated dynamically with new quotas that have an expiration date.
 */
class DynamicReservoir : public Reservoir {
public:
  explicit DynamicReservoir(uint32_t quota) : Reservoir(quota), can_borrow_(false) {}

  /**
   * If the reservoir quota TTL has lapsed (i.e. expired) we make an assumption that the next
   * refresh will have a non-zero quota, and therefore we can borrow a single sample from it. This
   * means that borrowing should only happen when the reservoir's quota is expired.
   */
  bool borrow(Envoy::MonotonicTime now) {
    Envoy::Thread::LockGuard lg(sync_);
    // if the quota is zero, then we can't borrow.
    if (quota_ == 0) {
      return false;
    }

    const auto diff = now - current_second_;
    if (diff > std::chrono::seconds(1)) {
      used_ = 0;
      current_second_ = now;
      can_borrow_ = true;
    }

    const bool borrow = can_borrow_;
    can_borrow_ = !can_borrow_;
    return borrow;
  }

  /**
   * Updates the reservoir's quota, expiration time and update interval.
   *
   * @param new_quota The number of requests to sample per second.
   * @param expires_at The time after which the new quota should not be used.
   * @param next_update_interval The duration of time in seconds after which, the existing quota
   * information would be considered stale (i.e. needs to be fetched from the service). This is
   * different from the quota expiration date.
   */
  void refresh(uint32_t new_quota, Envoy::SystemTime expires_at) {
    Envoy::Thread::LockGuard lg(sync_);
    quota_ = new_quota;
    expires_at_ = expires_at;
  }

  /**
   * Checks if the reservoir's current quota is expired given the input point-in-time.
   */
  bool isExpired(Envoy::SystemTime now) const {
    Envoy::Thread::LockGuard lg(sync_);
    return now >= expires_at_;
  }

private:
  Envoy::SystemTime expires_at_;
  bool can_borrow_;
};

} // namespace XRay
} // namespace Tracers
} // namespace Extensions
} // namespace Envoy
