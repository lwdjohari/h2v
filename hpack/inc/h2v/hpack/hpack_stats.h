#pragma once

#include <cstdint>

namespace h2v {
namespace hpack {

/// @brief Cumulative statistics for an HPACK codec instance.
/// @details Exposed via `HpackCodec::stats()` to monitor cache efficiency,
///          eviction behavior, and error rates. Users can sample these
///          periodically for observability or SLA enforcement.
struct HpackStats {
  /// Number of successful dynamic-table lookups (cache hits).
  uint64_t cache_hits = 0;

  /// Number of failed dynamic-table lookups (cache misses).
  uint64_t cache_misses = 0;

  /// Number of entries evicted from the dynamic table due to size limits.
  uint64_t evictions = 0;

  /// Number of encode/decode errors encountered.
  uint64_t error_count = 0;

  /// Total number of header fields encoded.
  uint64_t total_encoded_headers = 0;

  /// Total number of header fields decoded.
  uint64_t total_decoded_headers = 0;

  /// Total bytes processed (sum of all payloads encoded + decoded).
  uint64_t total_bytes_processed = 0;

  // Future extensions:
  // - Latency histograms (p95/p99) for encode/decode operations.
  // - Per-EntryType counters for breakdown by Indexed vs. Literal forms.
};

}  // namespace hpack
}  // namespace h2v