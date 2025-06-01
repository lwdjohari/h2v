
#include <algorithm>
#include <array>
#include <cstdint>
#include <stdexcept>
#include <vector>

#include "h2v/utils/byte_utils.h"

namespace h2v {
namespace stream {
class BitPacker {
 public:
  /// Write the low `bit_len` bits of `symbol` (≤30) into the stream, MSB-first.
  /// bit_len must be in [1..30].
  void WriteSymbol(uint32_t symbol, uint8_t bit_len) {
    if (bit_len == 0 || bit_len > 30)
      throw std::invalid_argument("bit_len must be 1..30");
    // mask to bit_len LSBs
    symbol &= (bit_len == 32 ? 0xFFFFFFFFu : ((1u << bit_len) - 1));
    WriteBitsBE(symbol, bit_len);
  }

  // Pad the *remaining* bits in the current byte using the MSB-bits of any
  // symbol (e.g. EOS), given as its LSB-aligned 'symbol' + full 'bit_len'. You
  // can pass in a symbol length >8 (e.g. EOS_LEN=30). Only the top (8-bit_pos_)
  // bits are emitted to fill out the byte.
  void PadWithSymbol(uint32_t symbol, uint8_t bit_len) {
    if (bit_len == 0 || bit_len > 32)
      throw std::invalid_argument("bit_len must be 1..32");
    if (bit_pos_ == 0)
      return;  // already aligned

    // mask to bit_len:
    symbol &= (bit_len == 32 ? 0xFFFFFFFFu : ((1u << bit_len) - 1));

    uint8_t need = 8 - bit_pos_;  // bits needed to fill current byte
    // take the highest 'need' bits of our symbol:
    uint8_t shift = bit_len - need;
    uint8_t pad = static_cast<uint8_t>((symbol >> shift) & ((1u << need) - 1));

    WriteBitsBE(pad, need);
  }

  /// Flush any partial byte (pads the trailing bits with zeros).
  void Flush() {
    if (bit_pos_ > 0) {
      buffer_.push_back(current_byte_);
      current_byte_ = 0;
      bit_pos_ = 0;
    }
  }

  // Access the packed data.
  const std::vector<uint8_t>& Data() const {
    return buffer_;
  }

  /// Returns a classic hex dump with offsets, hex bytes, and ASCII on the
  /// right.
  /// Each line shows `bytes_per_line` bytes (default 16).
  std::string HexDump() {
    return utils::byte::HexDump(buffer_);
  }

  /// Returns a hex+ASCII dump, grouping bytes into big-endian 16-bit words.
  /// Each line contains `words_per_line` words, plus “ | ASCII…” on the right.
  std::string HexDump16() {
    return utils::byte::HexDump16WithASCII(buffer_);
  }

 private:
  std::vector<uint8_t> buffer_;
  uint8_t current_byte_ = 0;  // accumulating byte
  uint8_t bit_pos_ = 0;       // bits written in current_byte_ (0..8)

  /// Core MSB-first writer: write `count` bits of `value` (already masked) into
  /// the buffer.
  void WriteBitsBE(uint32_t value, uint8_t count) {
    uint8_t bits_rem = count;
    while (bits_rem) {
      if (bit_pos_ == 8) {
        buffer_.push_back(current_byte_);
        current_byte_ = 0;
        bit_pos_ = 0;
      }
      uint8_t free_bits = 8 - bit_pos_;
      uint8_t to_write = std::min(free_bits, bits_rem);
      // grab the top `to_write` bits from value
      uint8_t shift = bits_rem - to_write;
      uint8_t chunk = (value >> shift) & ((1u << to_write) - 1);
      // position chunk MSB-first into current_byte_
      current_byte_ |= (chunk << (free_bits - to_write));
      bit_pos_ += to_write;
      bits_rem = shift;
    }
  }
};

}  // namespace stream
}  // namespace h2v
