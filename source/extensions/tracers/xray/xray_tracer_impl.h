#pragma once

#include "envoy/event/timer.h"
#include "envoy/server/instance.h"
#include "envoy/thread_local/thread_local.h"
#include "envoy/tracing/http_tracer.h"

#include "common/tracing/http_tracer_impl.h"

#include "extensions/tracers/xray/centralized_sampling.h"
#include "extensions/tracers/xray/poller.h"
#include "extensions/tracers/xray/tracer.h"
#include "extensions/tracers/xray/xray_configuration.h"

namespace Envoy {
namespace Extensions {
namespace Tracers {
namespace XRay {

class Driver : public Tracing::Driver, public Logger::Loggable<Logger::Id::tracing> {
public:
  Driver(const XRay::XRayConfiguration& config, Server::Instance& server);

  Tracing::SpanPtr startSpan(const Tracing::Config& config, Http::HeaderMap& request_headers,
                             const std::string& operation_name, Envoy::SystemTime start_time,
                             const Tracing::Decision tracing_decision) override;
  ~Driver() override;

private:
  struct TlsTracer : ThreadLocal::ThreadLocalObject {
    TlsTracer(TracerPtr tracer) : tracer_(std::move(tracer)) {}
    TracerPtr tracer_;
  };

  void onRulesFetched(const std::string& rules_json);

  XRayConfiguration xray_config_;
  SamplingStrategyPtr sampling_strategy_;
  CentralizedSamplingStrategyPtr centralized_sampling_strategy_;
  ThreadLocal::SlotPtr tls_slot_ptr_;
  SamplingRulesPoller rule_poller_;
  Event::TimerPtr rule_poller_timer_;
};

} // namespace XRay
} // namespace Tracers
} // namespace Extensions
} // namespace Envoy
