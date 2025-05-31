// include/h2v/hpack/dynamic_table.h
#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include "absl/container/node_hash_map.h"
#include "absl/strings/string_view.h"
#include "absl/synchronization/mutex.h"
#include "h2v/hpack/entry_type.h"
#include "h2v/hpack/error_tracer.h"
#include "h2v/hpack/hpack_stats.h"
#include "h2v/stream/raw_buffer.h"

namespace h2v {
namespace hpack {

/// @brief Dynamic table for HPACK: wire‐exact bytes + decoded entry cache.
class DynamicTable {
 public:
  struct Entry {
    absl::string_view raw_name, raw_value;
    std::string decoded_name, decoded_value;
    uint32_t index;  ///< 1-based HPACK index
    EntryType type;
  };

  explicit DynamicTable(std::size_t max_bytes) noexcept;
  ~DynamicTable();

  /// Lookup by raw name slice.
  std::shared_ptr<Entry> Find(absl::string_view name_slice) noexcept;

  /// Lookup by HPACK index (static table offset + dynamic index).
  std::shared_ptr<Entry> FindByIndex(uint32_t index) noexcept;

  /// Insert new entry, evicting oldest if needed.
  std::shared_ptr<Entry> Insert(absl::string_view name_slice,
                                absl::string_view value_slice,
                                std::string&& decoded_name,
                                std::string&& decoded_value,
                                EntryType type) noexcept;

  std::size_t BytesUsed() const noexcept;
  void Clear() noexcept;
  void SnapshotStats(HpackStats& out) const noexcept;
  /// @brief Dynamically change the maximum byte capacity and evict if needed.
  void SetMaxBytes(std::size_t new_max) noexcept;

 private:
  mutable absl::Mutex mutex_;
  stream::RawBuffer<> raw_buffer_;
  absl::node_hash_map<absl::string_view, std::shared_ptr<Entry>> cache_;
  std::vector<std::shared_ptr<Entry>> queue_;
  std::size_t head_ = 0, tail_ = 0;
  std::size_t max_bytes_, current_bytes_ = 0;
  HpackStats stats_;

  void EvictIfNeeded(std::size_t need) noexcept;
  void EvictOne() noexcept;
};

}  // namespace hpack
}  // namespace h2v
