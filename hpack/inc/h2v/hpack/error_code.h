#pragma once
#include <cstddef>
#include <cstdint>
namespace h2v {
namespace hpack {

namespace HPACK_ERR {

static constexpr int32_t NONE = 0;
static constexpr int32_t INPUT_NULL_PTR = 1;
static constexpr int32_t OUTPUT_NULL_PTR = 2;
static constexpr int32_t INPUT_SIZE_ZERO = 3;
static constexpr int32_t OUTPUT_SIZE_ZERO = 4;
static constexpr int32_t BUFFER_TO_SMALL = 5;
static constexpr int32_t INVALID_ARGS = 6;
static constexpr int32_t HUFFMAN_DECODE_INVALID_PREFIX_NIBBLE = 7;
static constexpr int32_t HUFFMAN_DECODE_INVALID_PREFIX_FBYTE = 8;
static constexpr int32_t HUFFMAN_DECODE_INVALID_EOS_PADDING_NIBBLE =9;
static constexpr int32_t HUFFMAN_DECODE_INVALID_EOS_PADDING_FBYTE = 10;

}  // namespace error_code
}  // namespace hpack
}  // namespace h2v