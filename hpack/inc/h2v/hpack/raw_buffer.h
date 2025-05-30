// include/hpack/raw_buffer.h
#pragma once

#include <cctype>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <memory>
#include <sstream>
#include <utility>

#include "absl/types/span.h"

namespace h2v {
namespace hpack {

/// @brief A resizable byte buffer with full allocator control,
///        avoiding any std::vector overhead.
/// @tparam Allocator  STL-compatible allocator for uint8_t.
template <typename Allocator = std::allocator<uint8_t>>
class RawBuffer {
 public:
  using allocator_type = Allocator;
  using value_type = uint8_t;

  /// @param alloc    allocator instance to use for all memory ops.
  /// @param initial_capacity  reserve this many bytes initially.
  explicit RawBuffer(const Allocator& alloc = Allocator(),
                     std::size_t initial_capacity = 0)
                  : alloc_(alloc), data_(nullptr), size_(0), capacity_(0) {
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
    clear_storage();
    alloc_ = std::move(o.alloc_);
    data_ = o.data_;
    size_ = o.size_;
    capacity_ = o.capacity_;
    o.data_ = nullptr;
    o.size_ = o.capacity_ = 0;
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

  /// Single-slice interface; can extend to multi-slice if needed.
  std::vector<absl::Span<const uint8_t>> slices() const noexcept {
    return {data()};
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
      oss << '|';
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
        oss << '|';
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

}  // namespace hpack
}  // namespace h2v