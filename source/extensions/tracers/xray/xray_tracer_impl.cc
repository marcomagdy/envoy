#include "extensions/tracers/xray/xray_tracer_impl.h"

namespace Envoy {
namespace Extensions {
namespace Tracers {
namespace XRay {

static const char DEFAULT_DAEMON_ENDPOINT[] = "127.0.0.1:2000";
static const char ENV_APPMESH_NODE_NAME[] = "APPMESH_VIRTUAL_NODE_NAME";
Driver::Driver(const XRayConfiguration& config, Server::Instance& server) : xray_config_(config) {

  const std::string daemon_endpoint =
      config.daemon_endpoint_.empty() ? DEFAULT_DAEMON_ENDPOINT : config.daemon_endpoint_;

  ENVOY_LOG(debug, "send X-Ray generated segments to daemon address on {}", daemon_endpoint);
  std::string span_name = server.localInfo().clusterName();
  // XRay segment name will be the combination of mesh name and the virtual node name.
  const auto vnode_name = std::getenv(ENV_APPMESH_NODE_NAME);
  if (vnode_name) {
    const auto parts = StringUtil::splitToken(vnode_name, "/", false /*keep_empty_string*/);
    if (parts.size() >= 4) {
      span_name = absl::StrCat(parts[1], "/", parts[3]);
    } else {
      ENVOY_LOG(debug,
                "Failed to construct an XRay span name from env. variable {} - using cluster's "
                "name instead",
                vnode_name);
    }
  } else if (!config.segment_name_.empty()) {
    span_name = config.segment_name_;
  }

  sampling_strategy_ = std::make_unique<XRay::SamplingStrategy>(server.random().random());
  tracer_.emplace(span_name, server.timeSource());
}

Tracing::SpanPtr Driver::startSpan(const Tracing::Config& config, Http::HeaderMap& request_headers,
                                   const std::string& operation_name, SystemTime start_time,
                                   const Tracing::Decision tracing_decision) {

  (void)config;
  (void)operation_name;
  (void)start_time;
  (void)tracing_decision;
  const SamplingRequest request{request_headers.Host()->value().getStringView(),
                                request_headers.Method()->value().getStringView(),
                                request_headers.Path()->value().getStringView()};

  if (!sampling_strategy_->sampleRequest(request)) {
    return nullptr;
  }

  return tracer_->startSpan();
}
} // namespace XRay
} // namespace Tracers
} // namespace Extensions
} // namespace Envoy
