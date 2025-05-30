#pragma once

#include <cstddef>

namespace h2v {
namespace hpack {

/// @brief Configuration for HPACK codec behavior and resource limits.
/// @details
///   - Controls the maximum dynamic table size (in bytes) to bound memory
///   usage.
///   - Controls the maximum header-list size to guard against oversized inputs.
///   - Selects strict vs. lenient error-handling mode.
/// @see RFC 7541 §4.2 “Size Update” for dynamic table semantics.
struct HpackConfig {
  /// Maximum total size (in octets) of the dynamic table.
  /// New insertions that would exceed this threshold trigger eviction
  /// or error (per strict vs. lenient mode).
  std::size_t max_dynamic_table_size_bytes = 4096;

  /// Maximum total size (in octets) of a header list (sum of name+value
  /// lengths). Decode operations on larger payloads fail immediately.
  std::size_t max_header_list_size_bytes = 16 * 1024;

  /// If true, any encode/decode error aborts the operation (fail-fast).
  /// If false, recoverable anomalies are logged and parsing continues.
  bool strict_mode = true;
};

}  // namespace hpack
}