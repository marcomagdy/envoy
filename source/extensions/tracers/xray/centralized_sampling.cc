#include "extensions/tracers/xray/centralized_sampling.h"

#include "envoy/common/time.h"
#include "envoy/runtime/runtime.h"

#include "common/common/lock_guard.h"
#include "common/http/exception.h"
#include "common/protobuf/utility.h"

#include "extensions/tracers/xray/localized_sampling.h"
#include "extensions/tracers/xray/reservoir.h"
#include "extensions/tracers/xray/sampling_strategy.h"

#include "absl/strings/string_view.h"

namespace Envoy {
namespace Extensions {
namespace Tracers {
namespace XRay {

namespace {

void fail(absl::string_view msg) {
  auto& logger = Logger::Registry::getLog(Logger::Id::tracing);
  ENVOY_LOG_TO_LOGGER(logger, error, "Failed to parse centralized sampling rules - {}", msg);
}
} // namespace

CentralizedSamplingManifest::CentralizedSamplingManifest(const std::string& sampling_rules_json) {

  constexpr auto SamplingRuleRecordsJsonKey = "SamplingRuleRecords";
  // constexpr auto FixedRateJsonKey = "FixedRate";
  constexpr auto HostJsonKey = "Host";
  constexpr auto HttpMethodJsonKey = "HTTPMethod";
  // constexpr auto PriorityJsonKey = "Priority";
  constexpr auto ReservoirSizeJsonKey = "ReservoirSize";
  constexpr auto UrlPathJsonKey = "URLPath";

  ProtobufWkt::Struct document;
  try {
    MessageUtil::loadFromJson(sampling_rules_json, document);
  } catch (EnvoyException& e) {
    fail("invalid JSON format");
  }

  const auto records_it = document.fields().find(SamplingRuleRecordsJsonKey);
  if (records_it == document.fields().end()) {
    fail("missing rule records");
    return;
  }

  if (records_it->second.kind_case() != ProtobufWkt::Value::KindCase::kListValue) {
    fail("rule records must be JSON array");
    return;
  }

  for (auto& el : records_it->second.list_value().values()) {
    if (el.kind_case() != ProtobufWkt::Value::KindCase::kStructValue) {
      fail("rule record must be a JSON object");
      return;
    }

    CentralizedSamplingRule rule;

    auto& rule_json = el.struct_value();
    const auto host_it = rule_json.fields().find(HostJsonKey);
    if (host_it != rule_json.fields().end()) {
      rule.setHost(host_it->second.string_value());
    }

    const auto http_method_it = rule_json.fields().find(HttpMethodJsonKey);
    if (http_method_it != rule_json.fields().end()) {
      rule.setHttpMethod(http_method_it->second.string_value());
    }

    const auto url_path_it = rule_json.fields().find(UrlPathJsonKey);
    if (url_path_it != rule_json.fields().end()) {
      rule.setUrlPath(url_path_it->second.string_value());
    }

    const auto reservoir_size_it = rule_json.fields().find(ReservoirSizeJsonKey);
    if (reservoir_size_it != rule_json.fields().end()) {
      rule.setReservoirSize(reservoir_size_it->second.number_value());
    }
  }
}

bool CentralizedSamplingStrategy::shouldTrace(const SamplingRequest& sampling_request) {
  (void)sampling_request;
  // TODO: not implemented yet
  return false;
}

void CentralizedSamplingStrategy::setManifest(const CentralizedSamplingManifest& manifest) {
  Envoy::Thread::LockGuard lg(manifest_sync_);
  manifest_.emplace(manifest);
}

} // namespace XRay
} // namespace Tracers
} // namespace Extensions
} // namespace Envoy
