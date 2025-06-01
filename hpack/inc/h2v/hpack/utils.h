#pragma once


#include <cstddef>
#include <string>


#include "absl/strings/string_view.h"

namespace h2v {
namespace hpack {
namespace util {

/// @brief Create an absl::string_view over raw bytes.
/// @param data  pointer to UTF-8 or binary data
/// @param len   number of bytes
/// @return      string_view pointing to [data, data+len)
inline absl::string_view ToStringView(const uint8_t* data,
                                      std::size_t len) noexcept {
  return absl::string_view(reinterpret_cast<const char*>(data), len);
}




}  // namespace util
}  // namespace hpack
}  // namespace h2v
