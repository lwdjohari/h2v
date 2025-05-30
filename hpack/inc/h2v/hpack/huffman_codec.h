// include/h2v/hpack/huffman_codec.h
#pragma once

#include <cstddef>
#include <cstdint>

#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "h2v/hpack/raw_buffer.h"

namespace h2v {
namespace hpack {

/// Subcodes for detailed Huffman errors.
enum class HuffmanSubcode : uint16_t {
  OutputOverflow = 1,
  DecodeError = 2,
};

/// @brief HuffmanCodec for HPACK literal compression.
class HuffmanCodec {
 public:
  /// @brief Huffman-encode `input` into `out` as an HPACK string literal
  /// (length prefix + Huffman data). Returns total bytes written or error.
  /// @param input         literal to compress
  /// @param out           buffer to append bytes
  static absl::StatusOr<std::size_t> Encode(absl::string_view input,
                                            RawBuffer<>& out) noexcept;

  /// @brief Huffman-encode `input` into `out` as an HPACK string literal
  /// (length prefix + Huffman data). Returns total bytes written or error.
  /// @param input         literal to compress
  /// @param out           buffer to append bytes
  static absl::StatusOr<std::size_t> Encode(const std::string& input,
                                            RawBuffer<>& out) noexcept;

  // Huffman Encoding Decoder implementations:
  // Flatmap FSM for Full byte-level decoding.
  // -----------------------------------------
  // HPACK’s Huffman codes map octets → bit-strings → octets.
  // The decoder implementation operates entirely on the octet stream,
  // emitting decoded octets and never attempting to interpret them as
  // characters. Whatever octets come out—valid byte. UTF-8, invalid UTF-8,
  // ISO-8859, binary garbage, the decoder will faithfully reconstruct them.
  // UTF-8 validity is a higher-level concern.
  // Empty input have terminated with an early check if (input.size()==0) return
  // "". That cleanly handles "" with zero bytes on the wire and no padding.
  // Invalid codes & padding Any lookup into kByteDecodeTable
  // that doesn’t correspond to a valid 8-bit extension of a codeword is caught
  // because we either assign emit_count=0 and next_state=error_state
  // (and later kAccepting will reject it).
  // We also explicitly check for emit_count==0xFF.
  // The final padding check uses exactly the number of trailing 1-bits needed
  // to align to a codeword boundary (via kStateDepth → pad_bits → kBitTable →
  // kAccepting), so any malformed padding—too few ones, too many, or spurious
  // symbol emissions—will be rejected.
  // All bit-patterns covered with generated a full 513×256 byte-FSM
  // plus a complete bit‐FSM for single‐bit steps,
  // every possible input byte sequence is handled.
  // There’s no unhandled branch or out-of-bounds lookup.
  // Performance unaffected by content Whether you’re decoding
  // ASCII, UTF-8 multibyte sequences, or arbitrary binary,
  // the hot loop is the same: one pointer-arithmetic lookup per byte,
  // two push_back calls at most, and a tiny O(1) padding check at the end.
  // So in short:
  // This Huffman decoder is encoding-agnostic. It will happily decode
  // any octet stream; it’s up to the caller to validate or interpret the
  // resulting bytes as UTF-8 (or not).
  // Edge cases:
  // - Empty strings → handled.
  // - Fully byte-aligned codewords → state==0 fast-path accepts with no padding
  // check.
  // - Mid-code endings → precise pad_bits → bit-FSM → accepting leaf check
  // ensures correct EOS.
  // - Invalid codewords or padding → reliably rejected via
  // InvalidArgumentError.

  /// @brief Decode `input` into `out`, returning total symbols decoded.
  /// @param ip         pointer to raw buffer
  /// @param ip_size    raw buffer size
  /// @param out        decoded string
  /// @param trace      turn on or off FSM Decoder tracer
  static absl::StatusOr<size_t> Decode(const uint8_t* ip, size_t ip_size,
                                       std::string& out,
                                       bool trace = false) noexcept;

  /// @brief Decode `input` into `out`, returning total symbols decoded.
  /// @param input      compressed input
  /// @param out        decoded string
  /// @param trace      turn on or off FSM Decoder tracer
  static absl::StatusOr<size_t> Decode(const RawBuffer<>& input,
                                       std::string& out,
                                       bool trace = false) noexcept;
};

}  // namespace hpack
}  // namespace h2v
