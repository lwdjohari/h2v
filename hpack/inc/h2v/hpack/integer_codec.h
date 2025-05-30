// include/hpack/integer_codec.h
#pragma once

#include <cstdint>

#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "h2v/hpack/error_tracer.h"
#include "h2v/hpack/raw_buffer.h"

namespace h2v {
namespace hpack {
namespace detail {

/// @brief Internal error codes for integer parsing failures.
enum class IntSubcode : uint16_t {
  IntOverflow =
      0x0001,  ///< Value exceeds maximum representable by prefix bits.
  InsufficientData =
      0x0002  ///< Unterminated varint: no byte with high bit = 0 found.
};

/// @brief Helper to compose an internal error code.
/// @param subcode  library-specific low-16 bits.
/// @return 32-bit domain+subcode error code.
constexpr uint32_t MakeIntError(IntSubcode subcode) noexcept {
  return make_error(0x0001, static_cast<uint16_t>(subcode));
}

}  // namespace detail

/// @brief Parse an HPACK integer from a buffer using N-bit prefix (per RFC 7541
/// §5.1).
/// @tparam PrefixBits  number of bits in the first byte used as the prefix
/// (1–8).
/// @param input        pointer to the buffer start (must be at least 1 byte).
/// @param end          pointer one past the buffer end.
/// @return the decoded integer on success, or a non-OK Status on
/// overflow/underrun.
/// @note Always noexcept; returns StatusOr to carry error subcodes for
/// diagnostics.
template <int PrefixBits>
inline absl::StatusOr<uint64_t> ParseInteger(const uint8_t* input,
                                             const uint8_t* end) noexcept {
  static_assert(PrefixBits >= 1 && PrefixBits <= 8,
                "PrefixBits must be in [1,8]");

  // Mask to extract the prefix bits from the first byte.
  constexpr uint8_t prefix_mask = static_cast<uint8_t>((1u << PrefixBits) - 1);
  uint8_t first = *input;
  uint64_t value = first & prefix_mask;

  // If value < mask, it's fully contained in the prefix.
  if (value < prefix_mask) {
    return value;
  }

  // Otherwise, parse continuation bytes (varint).
  uint64_t m = 0;
  const uint8_t* ptr = input + 1;
  while (ptr < end) {
    uint8_t byte = *ptr++;
    // Accumulate 7 bits per byte.
    if (m > (UINT64_MAX >> 7)) {
      // Overflow before shift.
      return absl::InvalidArgumentError(
          error_message(MakeIntError(detail::IntSubcode::IntOverflow),
                        "Integer overflow in HPACK prefix"));
    }
    value += static_cast<uint64_t>(byte & 0x7F) << m;
    if ((byte & 0x80) == 0) {
      return value;
    }
    m += 7;
  }

  // Ran out of input without termination.
  return absl::InvalidArgumentError(
      error_message(MakeIntError(detail::IntSubcode::InsufficientData),
                    "HPACK integer varint not terminated"));
}

/// @brief Encode an integer into an HPACK prefix form using N-bit prefix (per
/// RFC 7541 §5.1).
/// @tparam PrefixBits  number of bits in the first byte used as the prefix
/// (1–8).
/// @param value        integer to encode.
/// @param prefix_bits  bits [0…PrefixBits) of the first byte to set (e.g.
/// flags).
/// @param out          append-only buffer to write encoded bytes into.
/// @return true on success; false if out lacks capacity (caller should
/// reserve).
template <int PrefixBits>
inline bool EncodeInteger(uint64_t value, uint8_t prefix_bits,
                          RawBuffer<>& out) noexcept {
  static_assert(PrefixBits >= 1 && PrefixBits <= 8,
                "PrefixBits must be in [1,8]");

  constexpr uint8_t prefix_mask = static_cast<uint8_t>((1u << PrefixBits) - 1);
  uint8_t first_byte = prefix_bits & ~prefix_mask;

  if (value < prefix_mask) {
    // Entire value fits in prefix.
    first_byte |= static_cast<uint8_t>(value);
    if (auto ptr = out.append(1)) {
      *ptr = first_byte;
      return true;
    }
    return false;
  }

  // Value exceeds prefix; emit mask then varint.
  first_byte |= prefix_mask;
  if (auto ptr = out.append(1)) {
    *ptr = first_byte;
  } else {
    return false;
  }

  uint64_t remainder = value - prefix_mask;
  while (remainder >= 0x80) {
    uint8_t byte = static_cast<uint8_t>((remainder & 0x7F) | 0x80);
    if (auto ptr = out.append(1)) {
      *ptr = byte;
    } else {
      return false;
    }
    remainder >>= 7;
  }
  // Last byte without continuation bit.
  if (auto ptr = out.append(1)) {
    *ptr = static_cast<uint8_t>(remainder);
    return true;
  }
  return false;
}

}  // namespace hpack
}  // namespace h2v