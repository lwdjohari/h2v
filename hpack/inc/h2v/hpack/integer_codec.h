#pragma once
#include <cstddef>
#include <cstdint>

#include "h2v/hpack/error_code.h"
#include "h2v/stream/raw_buffer.h"
namespace h2v {
namespace hpack {
namespace integer_codec {

constexpr std::size_t ENCODE_MAX_BYTES = 6;

static stream::RawBuffer<> make_encoding_buffer() {
  return stream::RawBuffer({}, ENCODE_MAX_BYTES);
}

// Decodes an HPACK‐encoded integer from 'in'.
//   - 'prefix_bits' is the number of bits used for the integer in the first
//   byte (N).
//   - Returns a pair: { decoded_value, bytes_consumed }.
//   - Caller must ensure that 'in_len' ≥ 1, and that the buffer contains a
//   valid encoding.
//   - If the sequence is malformed (e.g. never-ending continuation bytes),
//   behavior is undefined.
//
// Example: to decode from a buffer where the first byte is 0x1F and N=5:
//   auto err_code = decode_integer(buffer, 1, 5, val, used);
//   // If buffer[0]=0x1F, then we know value ≥ 31, so we read more bytes.
inline int32_t DecodeInteger(const uint8_t* in, size_t in_size, int N,
                              uint32_t& out_val, size_t& out_size) noexcept {
  if (!in) {
    return HPACK_ERR::INPUT_NULL_PTR;
  }

  if (in_size == 0) {
    return HPACK_ERR::INPUT_SIZE_ZERO;
  }

  // 1) Mask for the lower N bits:
  const uint8_t prefix_mask = uint8_t((1u << N) - 1);

  // 2) Read first byte:
  const uint8_t first = in[0];
  uint32_t value = first & prefix_mask;

  // 3) If value < prefix_mask, no continuations—finished in 1 byte:
  if (value < prefix_mask) {
    out_val = value;
    out_size = 1;
    return HPACK_ERR::NONE;
  }

  // 4) Otherwise, we have to accumulate continuation bytes.
  //    We start with 'value = prefix_mask', and 'shift = 0':
  value = prefix_mask;
  uint32_t multiplier = 0;
  size_t idx = 1;

  // 5) Loop over continuation bytes (max 4 bytes for 32-bit ints, but we code
  // as “while”):
  //    Each continuation byte contributes (b & 0x7F) << multiplier, then
  //    multiplier += 7. We stop when we see a byte with MSB=0.
  while (true) {
    // We assume the caller has ensured in_len ≥ idx+1 for valid encodings.
    uint8_t b = in[idx];
    idx++;

    // Add the low 7 bits of b, shifted by 'multiplier':
    value += uint32_t(b & 0x7F) << multiplier;
    // If MSB is zero, this is the final byte:
    if ((b & 0x80) == 0) {
      break;
    }
    multiplier += 7;
    // (In proper HPACK you never encode > 32-bit values, so multiplier never
    // exceeds 28.)
  }

  out_val = value;
  out_size = idx;

  return HPACK_ERR::NONE;
}

// Encodes `value` as an HPACK integer using an N-bit prefix.
// `out`: buffer must be 6-bytes length or more
// `out_size`: return true bytes written after encoding
// `prefix_bits`: holds the high (8-N) bits that should be OR’d into the first
//  byte
// `N` : prefix bit (ie:5,6,7 or 8)
// `value`: value to encode
//  Returns the HPACK_ERR error code, if success return HPACK_ERR::NONE or 0.
//
// Example: to encode value=1337 with a 5-bit prefix (N=5) and prefix_bits=0x00
// (i.e. “literal + indexing” flag = 0):
//   encode_integer(out, 0x00, 5, 1337)
//   → writes: [ 0x1F, 0x9A, 0x0A ]  (i.e. 0b00011111, 0b10011010, 0b00001010)
//   → returns 3
//
// Implementation notes:
//  • We compute 'max = (1<<N)-1' once.
//  • If (value < max), we emit exactly one byte: (prefix_bits<<N) | value.
//  • Otherwise, we emit (prefix_bits<<N)|max, then loop:
//       while(value_remain >= 128) { *out++ = (value_remain & 0x7F) | 0x80;
//       value_remain >>= 7; } *out++ = uint8_t(value_remain);
inline int32_t ncodeInteger(uint8_t* out, size_t& out_size, uint8_t prefix_bits,
                            int N, uint32_t value) noexcept {
  if (!out) {
    return HPACK_ERR::OUTPUT_NULL_PTR;
  }

  //   'out' must point to at least ((value < (1<<N)) ? 1 : 1 + ceil((value -
  //   ((1<<N)-1)) / 7)) bytes of space.

  if (out_size < ENCODE_MAX_BYTES) {
    return HPACK_ERR::BUFFER_TO_SMALL;
  }

  // 1. Compute the maximum storable value in the N-bit prefix:
  const uint32_t max_prefix = (uint32_t(1) << N) - 1;

  // 2. If value fits entirely in the prefix, emit one byte:
  if (value < max_prefix) {
    // [ 0…(max_prefix-1) ] => single byte
    out[0] = uint8_t((prefix_bits << N) | value);
    out_size = 1;
    return HPACK_ERR::NONE;
  }

  // 3. Otherwise, emit the “all-ones” prefix byte, then spill the remainder:
  out[0] = uint8_t((prefix_bits << N) | max_prefix);
  uint32_t rem = value - max_prefix;
  size_t idx = 1;

  // 4. Emit continuation bytes: 7 bits at a time, MSB=1 if more follows.
  //    We use (rem & 0x7F) instead of rem % 128, and (rem >> 7) instead of rem
  //    / 128.
  while (rem >= 128) {
    // Take the low 7 bits, set MSB=1, write it:
    out[idx++] = uint8_t((rem & 0x7F) | 0x80);
    rem >>= 7;
  }
  // Last byte: MSB=0, low 7 bits = rem
  out[idx++] = uint8_t(rem);

  out_size = idx;
  return HPACK_ERR::NONE;
}

}  // namespace integer_codec
}  // namespace hpack
}  // namespace h2v