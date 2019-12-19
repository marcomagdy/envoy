#include "extensions/tracers/xray/poller.h"

#include "envoy/http/async_client.h"

#include "common/http/headers.h"
#include "common/http/message_impl.h"

namespace Envoy {
namespace Extensions {
namespace Tracers {
namespace XRay {

namespace {
constexpr auto GetSamplingRulesPath = "/GetSamplingRules";
class GetSamplingRulesCallbacks : public Http::AsyncClient::Callbacks {
public:
  GetSamplingRulesCallbacks(const Poller::PollCallback& callback) : callback_(callback) {}
  void onSuccess(Http::MessagePtr&& response) override { callback_(response->bodyAsString()); }
  void onFailure(Http::AsyncClient::FailureReason) override {
    // do nothing.
    // there's only one possible failure reason which is "The stream has been reset."
    // Since this is periodically polling, we will retry the call after a few minutes.
  }

private:
  Poller::PollCallback callback_;
};

} // namespace

void SamplingRulesPoller::poll(const PollCallback& callback) {
  Http::MessagePtr message = std::make_unique<Http::RequestMessageImpl>();
  message->headers().setMethod(Http::Headers::get().MethodValues.Post);
  message->headers().setPath(GetSamplingRulesPath);
  message->headers().setHost(endpoint_);
  // send() requires milliseconds (otherwise the call is ambiguous) so we have to do this seemingly
  // unnecessary dance.
  constexpr std::chrono::milliseconds timeout = std::chrono::minutes(2);
  GetSamplingRulesCallbacks cb(callback);
  http_client_.send(std::move(message), cb,
                    Http::AsyncClient::RequestOptions().setTimeout(timeout));
}
} // namespace XRay
} // namespace Tracers
} // namespace Extensions
} // namespace Envoy
