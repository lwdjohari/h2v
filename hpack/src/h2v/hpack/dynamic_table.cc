// src/h2v/hpack/dynamic_table.cc
#include "h2v/hpack/dynamic_table.h"

#include <cstring>

namespace h2v {
namespace hpack {

DynamicTable::DynamicTable(std::size_t max_bytes) noexcept
                : raw_buffer_{}, max_bytes_(max_bytes) {
  raw_buffer_.reserve(max_bytes_);
  queue_.reserve(1024);
}

DynamicTable::~DynamicTable() {
  Clear();
  raw_buffer_.reset();
}

std::shared_ptr<DynamicTable::Entry> DynamicTable::Find(
    absl::string_view name) noexcept {
  absl::MutexLock lk(&mutex_);
  auto it = cache_.find(name);
  if (it == cache_.end()) {
    stats_.cache_misses++;
    return nullptr;
  }
  stats_.cache_hits++;
  return it->second;
}

std::shared_ptr<DynamicTable::Entry> DynamicTable::FindByIndex(
    uint32_t idx) noexcept {
  absl::MutexLock lk(&mutex_);
  // dynamic indices start at static_table_size + 1
  // we store entries in queue_ in insertion order; their Entry::index
  // tracks absolute index
  for (auto& e : queue_) {
    if (e && e->index == idx) {
      stats_.cache_hits++;
      return e;
    }
  }
  stats_.cache_misses++;
  return nullptr;
}

std::shared_ptr<DynamicTable::Entry> DynamicTable::Insert(
    absl::string_view name_slice, absl::string_view value_slice,
    std::string&& dec_name, std::string&& dec_value, EntryType type) noexcept {
  absl::MutexLock lk(&mutex_);
  std::size_t need = name_slice.size() + value_slice.size();
  EvictIfNeeded(need);

  // append raw
  uint8_t* np = raw_buffer_.append(name_slice.size());
  if (!np) {
    stats_.error_count++;
    if (auto cb = GetErrorCallback())
      cb(0, make_error(0x1, 5), "OOM name");
    return nullptr;
  }
  std::memcpy(np, name_slice.data(), name_slice.size());
  uint8_t* vp = raw_buffer_.append(value_slice.size());
  if (!vp) {
    stats_.error_count++;
    if (auto cb = GetErrorCallback())
      cb(0, make_error(0x1, 5), "OOM value");
    return nullptr;
  }
  std::memcpy(vp, value_slice.data(), value_slice.size());

  // make entry
  auto e = std::make_shared<Entry>();
  e->raw_name =
      absl::string_view(reinterpret_cast<const char*>(np), name_slice.size());
  e->raw_value =
      absl::string_view(reinterpret_cast<const char*>(vp), value_slice.size());
  e->decoded_name = std::move(dec_name);
  e->decoded_value = std::move(dec_value);
  e->type = type;
  // index = static_table_size + cache_.size()+1
  e->index = static_cast<uint32_t>(/*static*/ 61 + cache_.size() + 1);

  // enqueue
  if (queue_.size() == 0 || tail_ == queue_.size())
    queue_.push_back(e);
  else
    queue_[tail_] = e;
  tail_ = (tail_ + 1) % queue_.capacity();

  cache_.emplace(e->raw_name, e);
  current_bytes_ += need;
  stats_.total_encoded_headers++;
  return e;
}

void DynamicTable::EvictIfNeeded(std::size_t need) noexcept {
  while (current_bytes_ + need > max_bytes_ && head_ != tail_) {
    EvictOne();
  }
}

void DynamicTable::EvictOne() noexcept {
  if (head_ == tail_)
    return;
  auto& e = queue_[head_];
  std::size_t sz = e->raw_name.size() + e->raw_value.size();
  cache_.erase(e->raw_name);
  head_ = (head_ + 1) % queue_.capacity();
  current_bytes_ = current_bytes_ > sz ? current_bytes_ - sz : 0;
  stats_.evictions++;
}

void DynamicTable::SetMaxBytes(std::size_t new_max) noexcept {
  absl::MutexLock lk(&mutex_);
  max_bytes_ = new_max;
  // Immediately evict if we're now over capacity
  EvictIfNeeded(0);
}

std::size_t DynamicTable::BytesUsed() const noexcept {
  absl::MutexLock lk(&mutex_);
  return current_bytes_;
}

void DynamicTable::Clear() noexcept {
  absl::MutexLock lk(&mutex_);
  cache_.clear();
  queue_.clear();
  head_ = tail_ = 0;
  current_bytes_ = 0;
  raw_buffer_.clear();
  stats_ = HpackStats{};
}

void DynamicTable::SnapshotStats(HpackStats& out) const noexcept {
  absl::MutexLock lk(&mutex_);
  out = stats_;
}

}  // namespace hpack
}  // namespace h2v
