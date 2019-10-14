#include "extensions/tracers/xray/util.h"

#include "absl/strings/ascii.h"

namespace Envoy {
namespace Extensions {
namespace Tracers {
namespace XRay {

bool wildcardMatch(absl::string_view pattern, absl::string_view input) {
  if (pattern.empty()) {
    return input.empty();
  }

  // Check the special case of a single * pattern, as it's common.
  constexpr char glob = '*';
  if (pattern.size() == 1 && pattern[0] == glob) {
    return true;
  }

  size_t i = 0, p = 0, i_star = input.size(), p_star = 0;
  while (i < input.size()) {
    if (p < pattern.size() && absl::ascii_tolower(input[i]) == absl::ascii_tolower(pattern[p])) {
      ++i;
      ++p;
    } else if (p < pattern.size() && '?' == pattern[p]) {
      ++i;
      ++p;
    } else if (p < pattern.size() && pattern[p] == glob) {
      i_star = i;
      p_star = p++;
    } else if (i_star != input.size()) {
      i = ++i_star;
      p = p_star + 1;
    } else {
      return false;
    }
  }

  while (p < pattern.size() && pattern[p] == glob) {
    ++p;
  }

  return p == pattern.size() && i == input.size();
}

} // namespace XRay
} // namespace Tracers
} // namespace Extensions
} // namespace Envoy
