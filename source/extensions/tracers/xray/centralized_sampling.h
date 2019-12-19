#pragma once

#include <memory>

#include "envoy/common/time.h"
#include "envoy/runtime/runtime.h"

#include "common/common/thread.h"

#include "extensions/tracers/xray/localized_sampling.h"
#include "extensions/tracers/xray/poller.h"
#include "extensions/tracers/xray/reservoir.h"
#include "extensions/tracers/xray/sampling_strategy.h"

#include "absl/strings/string_view.h"
#include "absl/types/optional.h"

namespace Envoy {
namespace Extensions {
namespace Tracers {
namespace XRay {

class CentralizedSamplingRule {
public:
  CentralizedSamplingRule() : reservoir_(0 /*quota*/) {}
  CentralizedSamplingRule(int64_t priority, uint64_t quota, double rate)
      : priority_(priority), rate_(rate), reservoir_(quota) {}

  bool appliesTo(const SamplingRequest& request) const;

  /**
   * Set the hostname to match against.
   * This value can contain wildcard characters such as '*' or '?'.
   */
  void setHost(absl::string_view host) { host_ = std::string(host); }

  /**
   * Set the HTTP method to match against.
   * This value can contain wildcard characters such as '*' or '?'.
   */
  void setHttpMethod(absl::string_view http_method) { http_method_ = std::string(http_method); }

  /**
   * Set the URL path to match against.
   * This value can contain wildcard characters such as '*' or '?'.
   */
  void setUrlPath(absl::string_view url_path) { url_path_ = std::string(url_path); }

  void setPriority(int64_t priority) { priority_ = priority; }

  void setRate(double rate) { rate_ = rate; }

  void setReservoirSize(uint32_t size) { reservoir_ = DynamicReservoir(size); }

private:
  std::string host_;
  std::string http_method_;
  std::string url_path_;
  int64_t priority_;
  double rate_;
  DynamicReservoir reservoir_;
};

class CentralizedSamplingManifest {
public:
  explicit CentralizedSamplingManifest(const std::string& sampling_rules_json);

private:
  std::vector<CentralizedSamplingRule> rules_;
};

/**
 * Provides dynamic polling for sampling rules from X-Ray.
 */
class CentralizedSamplingStrategy : public SamplingStrategy {
public:
  CentralizedSamplingStrategy(const std::string& sampling_rules_json, Runtime::RandomGenerator& rng,
                              TimeSource& time_source)
      : SamplingStrategy(rng), localized_strategy_(sampling_rules_json, rng, time_source) {}

  /**
   * Determines if an incoming request matches one of the sampling rules in the manifest.
   * If a match is found, then the request might be traced based on the sampling percentages etc.
   * determined by the matching rule.
   */
  bool shouldTrace(const SamplingRequest& sampling_request) override;

  void setManifest(const CentralizedSamplingManifest& manifest);

private:
  LocalizedSamplingStrategy localized_strategy_;
  absl::optional<CentralizedSamplingManifest> manifest_;
  mutable Envoy::Thread::MutexBasicLockable manifest_sync_;
};

using CentralizedSamplingStrategyPtr = std::unique_ptr<CentralizedSamplingStrategy>;

} // namespace XRay
} // namespace Tracers
} // namespace Extensions
} // namespace Envoy
