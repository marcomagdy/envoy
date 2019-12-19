#pragma once

#include <memory>

#include "envoy/common/pure.h"
#include "envoy/http/async_client.h"

namespace Envoy {
namespace Extensions {
namespace Tracers {
namespace XRay {

class Poller {
public:
  using PollCallback = std::function<void(const std::string&)>;
  virtual void poll(const PollCallback& callback);
  virtual ~Poller() = default;
};

class SamplingRulesPoller : public Poller {
public:
  explicit SamplingRulesPoller(const std::string& endpoint, Http::AsyncClient& http_client)
      : endpoint_(endpoint), http_client_(http_client){};
  void poll(const PollCallback& callback) override;

private:
  const std::string endpoint_;
  Http::AsyncClient& http_client_;
};

using SamplingRulesPollerPtr = std::unique_ptr<SamplingRulesPoller>;

} // namespace XRay
} // namespace Tracers
} // namespace Extensions
} // namespace Envoy
