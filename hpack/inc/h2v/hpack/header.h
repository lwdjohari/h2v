// include/hpack/header.h
#pragma once

#include "absl/strings/string_view.h"

namespace h2v {
namespace hpack {

/// @brief Represents an HTTP header name/value pair.
/// @details Both `name` and `value` are views into backing storage:
///   - During decode: slices from the HPACK payload ring buffer.
///   - During encode: views over user-supplied strings.
struct Header {
  /// The header field name (e.g. ":method", "content-type").
  absl::string_view name;

  /// The header field value (e.g. "GET", "application/json").
  absl::string_view value;
};

}  // namespace hpack
}  // namespace h2v