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
///
/// - `use_highway = true` attempts SIMD via Highway stubs; currently no-op.
/// - Otherwise, falls back to guaranteed-correct scalar bit-by-bit logic.
class HuffmanCodec {
 public:
  /// @brief Huffman-encode `input` into `out` as an HPACK string literal
  /// (length prefix + Huffman data). Returns total bytes written or error.
  /// @param input         literal to compress
  /// @param out           buffer to append bytes
  /// @param use_highway   if true, try Highway API first
  static absl::StatusOr<std::size_t> Encode(absl::string_view input,
                                            RawBuffer<>& out,
                                            bool use_highway = false) noexcept;

  /// @brief Huffman-encode `input` into `out` as an HPACK string literal
  /// (length prefix + Huffman data). Returns total bytes written or error.
  /// @param input         literal to compress
  /// @param out           buffer to append bytes
  /// @param use_highway   if true, try Highway API first
  absl::StatusOr<std::size_t> Encode(const std::string& input, RawBuffer<>& out,
                                     bool use_highway) noexcept;
                                     
  /// @brief Decode `input` into `out`, returning total symbols decoded.
  /// @param input         compressed input
  /// @param out           buffer to append decoded bytes
  /// @param use_highway   if true, try Highway API first
  static absl::StatusOr<size_t> Decode(absl::string_view input,
                                       RawBuffer<>& out,
                                       bool use_highway = true) noexcept;

 private:
  // RFC 7541 Appendix B: (code, bit_length) for each symbol [0..255]
  static const uint32_t kCode[256];
  static const uint8_t kLen[256];

  // RFC 7541 Appendix B decoding state table
  static const uint16_t kDecodeTable[][2];  // [state][bit]

  // Highway stubs; currently no-ops
  static bool highwayEncode(absl::string_view /*input*/,
                            RawBuffer<>& /*out*/) noexcept {
    return false;
  }
  static absl::StatusOr<size_t> highwayDecode(absl::string_view /*in*/,
                                              RawBuffer<>& /*out*/) noexcept {
    return absl::InvalidArgumentError("Highway not implemented");
  }
};

}  // namespace hpack
}  // namespace h2v
