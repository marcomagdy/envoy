#pragma once

#include "envoy/runtime/runtime.h"
#include "extensions/tracers/xray/localized_sampling.h"
#include "extensions/tracers/xray/reservoir.h"
#include "extensions/tracers/xray/sampling_strategy.h"

namespace Envoy {
namespace Extensions {
namespace Tracers {
namespace XRay {

class CentralizedSamplingRule {
public:
    CentralizedSamplingRule() = default;
private:
  std::string rule_name_;
  int64_t priority_;
  double rate_;
  CentralizedReservoir reservoir_;
};

class CentralizedSamplingManifest {
public:
  explicit CentralizedSamplingManifest(const std::string& sampling_rules_json);

private:
  CentralizedSamplingRule default_rule_;
  std::vector<CentralizedSamplingRule> rules_;
};

/**
 * Provides dynamic polling for sampling rules from X-Ray.
 */
class CentralizedSamplingStrategy : public SamplingStrategy {
public:
  CentralizedSamplingStrategy(const std::string& sampling_rules_json, Runtime::RandomGenerator& rng,
                              TimeSource& time_source)
      : SamplingStrategy(rng), time_source_(time_source),
        localized_strategy_(sampling_rules_json, rng, time_source) {}

  /**
   * Determines if an incoming request matches one of the sampling rules in the manifest.
   * If a match is found, then the request might be traced based on the sampling percentages etc.
   * determined by the matching rule.
   */
  bool shouldTrace(const SamplingRequest& sampling_request) override;

private:
  TimeSource& time_source_;
  LocalizedSamplingStrategy localized_strategy_;
  std::unique_ptr<CentralizedSamplingManifest> manifest_;
};

} // namespace XRay
} // namespace Tracers
} // namespace Extensions
} // namespace Envoy
