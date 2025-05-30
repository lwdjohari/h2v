// src/integer_codec.cc
#include "h2v/hpack/integer_codec.h"

namespace h2v {
namespace hpack {

// Explicit instantiations for common prefix sizes to aid LTO/PGO visibility
template absl::StatusOr<uint64_t> ParseInteger<5>(const uint8_t* input,
                                                  const uint8_t* end) noexcept;

template absl::StatusOr<uint64_t> ParseInteger<6>(const uint8_t* input,
                                                  const uint8_t* end) noexcept;

template absl::StatusOr<uint64_t> ParseInteger<7>(const uint8_t* input,
                                                  const uint8_t* end) noexcept;

template absl::StatusOr<uint64_t> ParseInteger<8>(const uint8_t* input,
                                                  const uint8_t* end) noexcept;

template bool EncodeInteger<5>(uint64_t value, uint8_t prefix_bits,
                               RawBuffer<>& out) noexcept;

template bool EncodeInteger<6>(uint64_t value, uint8_t prefix_bits,
                               RawBuffer<>& out) noexcept;

template bool EncodeInteger<7>(uint64_t value, uint8_t prefix_bits,
                               RawBuffer<>& out) noexcept;

template bool EncodeInteger<8>(uint64_t value, uint8_t prefix_bits,
                               RawBuffer<>& out) noexcept;

}  // namespace hpack
}  // namespace h2v