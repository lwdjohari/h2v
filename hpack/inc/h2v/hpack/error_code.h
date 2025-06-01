#pragma once
#include <cstddef>
#include <cstdint>

extern "C" inline const char H2V_HPACK_VERSION[]
    __attribute__((used, visibility("default"))) = "H2V_HPACK_VERSION: v0.6.1";

#if defined(H2V_HPACK_HUFFMAN_ENCODER_USE_BIT_OP) && \
    (H2V_HPACK_HUFFMAN_ENCODER_USE_BIT_OP == 1)
extern "C" inline const char H2V_HPACK_HUFFMAN_ENCODER[]
    __attribute__((used, visibility("default"))) =
        "H2V_HPACK_HUFFMAN_ENCODER: 0x1 - Bits operations";
#else
extern "C" inline const char H2V_HPACK_HUFFMAN_ENCODER[]
    __attribute__((used, visibility("default"))) =
        "H2V_HPACK_HUFFMAN_ENCODER: 0x0 - Precomputed Encode Table";
#endif

#if defined(H2V_HPACK_HUFFMAN_DECODER_USE_FULLBYTE) && \
    (H2V_HPACK_HUFFMAN_DECODER_USE_FULLBYTE == 1)
extern "C" inline const char H2V_HPACK_HUFFMAN_DECODER[]
    __attribute__((used, visibility("default"))) =
        "H2V_HPACK_HUFFMAN_DECODER: 0x1 - Precomputed Full-Byte FSM";
#else
extern "C" inline const char H2V_HPACK_HUFFMAN_DECODER[]
    __attribute__((used, visibility("default"))) =
        "H2V_HPACK_HUFFMAN_DECODER: 0x0 - Precomputed 4Bit Nibble FSM";
#endif

namespace h2v {
namespace hpack {

using HpackErrorCode = int32_t;

namespace HPACK_ERR {

static constexpr HpackErrorCode NONE = 0;
static constexpr HpackErrorCode INPUT_NULL_PTR = 1;
static constexpr HpackErrorCode OUTPUT_NULL_PTR = 2;
static constexpr HpackErrorCode INPUT_SIZE_ZERO = 3;
static constexpr HpackErrorCode OUTPUT_SIZE_ZERO = 4;
static constexpr HpackErrorCode BUFFER_TO_SMALL = 5;
static constexpr HpackErrorCode INVALID_ARGS = 6;
static constexpr HpackErrorCode HUFFMAN_DECODE_INVALID_PREFIX_NIBBLE = 7;
static constexpr HpackErrorCode HUFFMAN_DECODE_INVALID_PREFIX_FBYTE = 8;
static constexpr HpackErrorCode HUFFMAN_DECODE_INVALID_EOS_PADDING_NIBBLE = 9;
static constexpr HpackErrorCode HUFFMAN_DECODE_INVALID_EOS_PADDING_FBYTE = 10;
static constexpr HpackErrorCode HPACK_HUFFMAN_DECODE_PAD_INVALID = 11;
static constexpr HpackErrorCode HPACK_HUFFMAN_DECODE_INVALID_EOS = 12;

}  // namespace HPACK_ERR
}  // namespace hpack
}  // namespace h2v