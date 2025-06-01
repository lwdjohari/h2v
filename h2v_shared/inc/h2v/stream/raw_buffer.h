// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Linggawasistha Djohari
// <linggawasistha.djohari@outlook.com>

#pragma once

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <memory>
#include <sstream>
#include <utility>

#include "absl/types/span.h"

namespace h2v {
namespace stream {

using RawBufferErrorCode = int32_t;
namespace RAW_BUFFER_ERR {

static constexpr RawBufferErrorCode NONE = 0;
static constexpr RawBufferErrorCode INVALID_ARGS = 1;
static constexpr RawBufferErrorCode OUT_OF_RANGE = 2;
static constexpr RawBufferErrorCode OVERRUN = 3;
static constexpr RawBufferErrorCode NULL_PTR = 4;
static constexpr RawBufferErrorCode INVARIANT_VIOLATION = 5;
}  // namespace RAW_BUFFER_ERR

/// @brief A resizable byte buffer with full allocator control
///        and absl::span integration for safe byte operations.
///        Avoiding any std::vector overhead or raw array alloc.
/// @tparam Allocator  STL-compatible allocator for uint8_t.
template <typename Allocator = std::allocator<uint8_t>>
class RawBuffer {
 public:
  using allocator_type = Allocator;
  using value_type = uint8_t;

  // explicit RawBuffer(const Allocator& alloc = Allocator(),
  //                    std::size_t initial_capacity = 0)
  //                 : alloc_(alloc), data_(nullptr), size_(0), capacity_(0) {
  //   reserve(initial_capacity);
  // }

  /// @param alloc    allocator instance to use for all memory ops.
  /// @param initial_capacity  reserve this many bytes initially.
  explicit RawBuffer(Allocator alloc = Allocator(),
                     std::size_t initial_capacity = 0)
                  : alloc_(std::move(alloc)),
                    data_(nullptr),
                    size_(0),
                    capacity_(0) {
    reserve(initial_capacity);
  }

  RawBuffer(RawBuffer&& o) noexcept
                  : alloc_(std::move(o.alloc_)),
                    data_(o.data_),
                    size_(o.size_),
                    capacity_(o.capacity_) {
    o.data_ = nullptr;
    o.size_ = o.capacity_ = 0;
  }

  RawBuffer& operator=(RawBuffer&& o) noexcept {
    if (this != &o) {
      clear_storage();
      alloc_ = std::move(o.alloc_);
      data_ = o.data_;
      size_ = o.size_;
      capacity_ = o.capacity_;
      o.data_ = nullptr;
      o.size_ = o.capacity_ = 0;
    }
    return *this;
  }

  ~RawBuffer() {
    clear_storage();
  }

  /// Reserve at least `new_cap` bytes. Never shrinks.
  void reserve(std::size_t new_cap) {
    if (new_cap <= capacity_)
      return;
    // allocate new block
    auto traits = std::allocator_traits<Allocator>{};
    uint8_t* new_data = traits.allocate(alloc_, new_cap);
    // copy existing bytes
    if (data_) {
      std::memcpy(new_data, data_, size_);
      traits.deallocate(alloc_, data_, capacity_);
    }
    data_ = new_data;
    capacity_ = new_cap;
  }

  /// Append `n` bytes and return pointer to first uninitialized byte,
  /// or nullptr on allocation failure.
  uint8_t* append(std::size_t n) noexcept {
    if (size_ + n > capacity_) {
      // grow (e.g. doubling strategy)
      std::size_t new_cap = std::max(capacity_ * 2, size_ + n);
      try {
        reserve(new_cap);
      } catch (...) {
        return nullptr;
      }
    }
    uint8_t* ptr = data_ + size_;
    size_ += n;
    return ptr;
  }

  absl::Span<const uint8_t> data() const noexcept {
    return {data_, size_};
  }

  const uint8_t* raw() const noexcept {
    return data_;
  }

  uint8_t* mutable_raw() noexcept {
    return data_;
  }

  absl::Span<uint8_t> mutable_data() noexcept {
    return {data_, size_};
  }

  std::size_t size() const noexcept {
    return size_;
  }
  std::size_t capacity() const noexcept {
    return capacity_;
  }

  /// Clear contents but retain capacity.
  void clear() noexcept {
    size_ = 0;
  }

  /// Release all memory.
  void reset() noexcept {
    clear_storage();
    data_ = nullptr;
    size_ = capacity_ = 0;
  }

  /**
   * Return a single contiguous span of `len` bytes starting at `pos`.
   *
   * If end_on_capacity == false, we require pos+len ≤ size_.
   * If end_on_capacity == true, we require pos+len ≤ capacity_.
   *
   * This function throws BufferError on any invalid argument or invariant
   * violation.
   *
   * @param len               Num of bytes to include in the slice; must be ≥ 0.
   * @param pos               Starting byte‐offset (default=0).
   * @param ec return RAW_BUFFER_ERR::NONE error code when success
   * @param end_on_capacity   If true, slice bounds are checked against
   * capacity_; otherwise against size_.
   * @return                  absl::Span<const uint8_t> covering [pos..pos+len).
   * @exception error as RAW_BUFFER_ERR:
   *   - slice_size == 0 (INVALID),
   *   - pos > limit (OUT_OF_RANGE),
   *   - len > (limit − pos) (OVERRUN),
   *   - buffer_ == nullptr while limit > 0 (NULL_PTR),
   *   - capacity_ < size_ (INV_VIOLATION).
   */
  absl::Span<const uint8_t> slice(std::size_t len, std::size_t pos,
                                  RawBufferErrorCode& ec,
                                  bool end_on_capacity = false) const {
    // 1. Which boundary to use: size_ vs capacity_.
    const std::size_t limit = end_on_capacity ? capacity_ : size_;

    // 2. Check slice_size (len) is not negative.
    //    (std::size_t is unsigned, so only check for len == 0 below.)
    // 3. If limit is 0, the only valid (pos,len) is (0,0).
    if (limit == 0) {
      if (len != 0 || pos != 0) {
        ec = RAW_BUFFER_ERR::OUT_OF_RANGE;
      }
      // Return an empty span
      return absl::Span<const uint8_t>();
    }

    // 4. Now limit > 0. Ensure buffer_ is not null.
    if (data_ == nullptr) {
      ec = RAW_BUFFER_ERR::NULL_PTR;
    }

    // 5. Ensure slice_size (len) is > 0.
    if (len == 0) {
      ec = RAW_BUFFER_ERR::INVALID_ARGS;
    }

    // 6. Check invariant: capacity_ ≥ size_.
    if (capacity_ < size_) {
      ec = RAW_BUFFER_ERR::INVARIANT_VIOLATION;
    }

    // 7. pos must be < limit.
    if (pos >= limit) {
      ec = RAW_BUFFER_ERR::OUT_OF_RANGE;
    }

    // 8. rem = limit - pos: bytes available from pos to end.
    const std::size_t rem = limit - pos;

    // 9. len must not exceed rem.
    if (len > rem) {
      ec = RAW_BUFFER_ERR::OVERRUN;
    }

    // 10. All checks passed → return the span.
    return absl::Span<const uint8_t>(data_ + pos, len);
  }

  /**
   * Multi‐slice
   *
   *  - slice_size must be > 0, otherwise std::invalid_argument.
   *  - If data() == nullptr but size() > 0 or capacity() > 0, return empty
   * vector.
   *  - If capacity() < size(), return empty vector.
   *  - Otherwise, splits [0..total_len) into spans of up to slice_size bytes.
   *
   * @param slice_size       Maximum length of each span (must be >= 1).
   * @param slice_on_capacity true split up to capacity(), false up to size().
   * @param ec return RAW_BUFFER_ERR::NONE error code when success
   * @return Vector of spans, each pointing to a contiguous chunk of the buffer.
   * @throws none - error is passing out through `ec`.
   */
  std::vector<absl::Span<const uint8_t>> slices(
      std::size_t slice_size, bool slice_on_capacity,
      RawBufferErrorCode& ec) const noexcept {
    // 1. Must request a nonzero slice_size
    // 2. If we have no allocated buffer
    //    (capacity_ == 0 and size_ == 0), return empty.
    if (slice_size == 0 || (capacity_ == 0 && size_ == 0)) {
      ec = RAW_BUFFER_ERR::INVALID_ARGS;
      return {};
    }

    // 3. If buffer_ == nullptr but either size_ > 0 or capacity_ > 0,
    //    that’s an  error.
    if (data_ == nullptr) {
      ec = RAW_BUFFER_ERR::NULL_PTR;
      return {};
    }

    // 4. Ensure capacity_ >= size_.
    //    If not, invariant violated.
    if (capacity_ < size_) {
      ec = RAW_BUFFER_ERR::INVARIANT_VIOLATION;
      return {};
    }

    // 5. Decide how far to slice: either up to size_ or up to capacity_.
    const std::size_t total_len = slice_on_capacity ? capacity_ : size_;

    // 6. If nothing to slice
    //    (e.g., size_=0 and slice_on_capacity=false), return empty.
    if (total_len == 0) {
      ec = RAW_BUFFER_ERR::NONE;
      return {};
    }

    // 8. Reserve enough room for all potential sub‐slices.
    //    ceil(total_len / slice_size) = (total_len + slice_size - 1) /
    //    slice_size
    std::size_t max_slices = (total_len + slice_size - 1) / slice_size;

    std::vector<absl::Span<const uint8_t>> out;
    out.reserve(max_slices);

    // 9. Walk through the buffer, slicing in chunks of up to slice_size bytes.
    std::size_t offset = 0;
    while (offset < total_len) {
      std::size_t chunk_len = std::min(slice_size, total_len - offset);
      out.emplace_back(absl::Span<const uint8_t>(data_ + offset, chunk_len));
      offset += chunk_len;
    }

    ec = RAW_BUFFER_ERR::NONE;
    return out;
  }

  /// Simple hex dump on one line, with optional ASCII aligned in its own
  /// column.
  /// - Lowercase hex bytes, 2 digits each, separated by spaces
  /// - ASCII column starts at a fixed column for uniformity
  /// - dump_all: uses capacity() for the hex area width; else size()
  std::string HexDump(bool dump_all = false, bool with_ascii = false) const {
    const uint8_t* ptr = data_;
    size_t len = dump_all ? capacity_ : size_;
    size_t used = dump_all ? size_ : size_;  // how many bytes to actually dump?
    std::ostringstream oss;
    oss << std::hex << std::nouppercase << std::setfill('0');

    // 1) Hex bytes
    for (size_t i = 0; i < used; ++i) {
      oss << std::setw(2) << int(ptr[i]);
      if (i + 1 < used)
        oss << ' ';
    }

    // 2) Pad to fixed hex area width so ASCII lines up
    if (with_ascii) {
      size_t hex_area_bytes = dump_all ? capacity_ : used;
      // each byte takes 2 hex digits + 1 space except last -> width =
      // hex_area_bytes*3 - 1
      size_t hex_area_width =
          hex_area_bytes * 3 > 0 ? (hex_area_bytes * 3 - 1) : 0;
      size_t current_width = used * 3 > 0 ? (used * 3 - 1) : 0;
      if (hex_area_width > current_width) {
        oss << std::string(hex_area_width - current_width, ' ');
      }
      // now ASCII column
      oss << "  |";
      for (size_t i = 0; i < used; ++i) {
        unsigned char c = ptr[i];
        oss << (std::isprint(c) ? char(c) : '.');
      }
      // oss << '|';
    }

    oss << std::dec;
    return oss.str();
  }

  /// RFC-style 16-byte hexdump with aligned ASCII column.
  /// Always emits 8 words (even if past size, filled with spaces) so ASCII
  /// lines up.
  std::string HexDump16(bool dump_all = false, bool with_ascii = false) const {
    const uint8_t* ptr = data_;
    size_t len = dump_all ? capacity_ : size_;
    std::ostringstream oss;
    oss << std::hex << std::nouppercase << std::setfill('0');

    constexpr size_t LINE_BYTES = 16;
    constexpr size_t WORD_BYTES = 2;
    constexpr size_t WORDS_PER_LINE = LINE_BYTES / WORD_BYTES;

    // Width of hex area: 8 words × 4 hex digits + 7 spaces = 8*4 + 7 = 39 chars
    constexpr size_t HEX_AREA_WIDTH = WORDS_PER_LINE * 4 + (WORDS_PER_LINE - 1);

    for (size_t offset = 0; offset < len; offset += LINE_BYTES) {
      size_t line_end = std::min(offset + LINE_BYTES, len);

      // 1) Hex words (always emit 8 slots)
      for (size_t w = 0; w < WORDS_PER_LINE; ++w) {
        size_t i = offset + w * WORD_BYTES;
        if (i < line_end) {
          // there is at least one real byte here
          uint16_t word = uint16_t(ptr[i]) << 8;
          if (i + 1 < line_end)
            word |= ptr[i + 1];
          oss << std::setw(4) << word;
        } else {
          // past end → emit 4 spaces
          oss << "    ";
        }
        if (w + 1 < WORDS_PER_LINE)
          oss << ' ';
      }

      // 2) Optional ASCII, aligned at column HEX_AREA_WIDTH + 2
      if (with_ascii) {
        oss << "  |";
        for (size_t i = offset; i < line_end; ++i) {
          char c = ptr[i];
          oss << (std::isprint(static_cast<unsigned char>(c)) ? c : '.');
        }
        // oss << '|';
      }

      // 3) Newline except after last line
      if (offset + LINE_BYTES < len)
        oss << '\n';
    }

    return oss.str();
  }

 private:
  Allocator alloc_;
  uint8_t* data_;
  std::size_t size_, capacity_;

  void clear_storage() noexcept {
    if (data_) {
      std::allocator_traits<Allocator>::deallocate(alloc_, data_, capacity_);
    }
  }
};

template <typename Allocator = std::allocator<uint8_t>>
inline static RawBuffer<Allocator> make_raw_buffer_from_copy(
    const uint8_t* ptr, std::size_t size,
    Allocator alloc = Allocator()) noexcept {
  if (ptr == nullptr) {
    return RawBuffer<Allocator>{};
  }

  if (size == 0) {
    // nothing to do, size_ stays 0
    return RawBuffer<Allocator>{};
  }

  auto out = RawBuffer(alloc, size);

  memcpy(out.mutable_raw(), ptr, size);
  out.append(size);

  return std::move(out);
}

}  // namespace stream
}  // namespace h2v