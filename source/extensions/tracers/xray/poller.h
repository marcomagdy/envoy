#pragma once

#include <memory>

#include "envoy/event/timer.h"
#include "envoy/http/async_client.h"

namespace Envoy {
namespace Extensions {
namespace Tracers {
namespace XRay {

class Poller {
public:
  virtual void poll();
  virtual ~Poller() = default;
};

class SamplingRulesPoller : public Poller {
public:
  SamplingRulesPoller();
  void poll() override;

private:
  Http::AsyncClient& http_client_;
  Event::TimerPtr flush_timer_;
};

using SamplingRulesPollerPtr = std::unique_ptr<SamplingRulesPoller>;

} // namespace XRay
} // namespace Tracers
} // namespace Extensions
} // namespace Envoy
