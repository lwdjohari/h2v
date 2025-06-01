// include/h2v/hpack/huffman_codec.h
#pragma once

#include <cstddef>
#include <cstdint>

#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "h2v/hpack/error_code.h"
#include "h2v/stream/raw_buffer.h"

#if defined(H2V_HPACK_HUFFMAN_ENCODER_USE_BIT_OP) && \
    (H2V_HPACK_HUFFMAN_ENCODER_USE_BIT_OP == 1)
#include "h2v/hpack/huffman_table.h"
#endif

#if !defined(H2V_HPACK_HUFFMAN_ENCODER_USE_BIT_OP) || \
    (H2V_HPACK_HUFFMAN_ENCODER_USE_BIT_OP == 0)
#include "h2v/hpack/generated/huffman_byte_table_encode.h"
#endif

#if !defined(H2V_HPACK_HUFFMAN_DECODER_USE_FULLBYTE) || \
    (H2V_HPACK_HUFFMAN_DECODER_USE_FULLBYTE == 0)
#include "h2v/hpack/generated/huffman_byte_table_nibble.h"
#endif

namespace h2v {
namespace hpack {
namespace huffman {

inline static stream::RawBuffer<> make_decode_buffer(size_t coded_size) {
  // worst-case: every symbol 30 bits + up to 7 pad bits
  return stream::RawBuffer<>({}, (coded_size * 30 + 7) / 8);
}

inline static stream::RawBuffer<> make_encode_buffer(size_t uncoded_size) {
  return stream::RawBuffer<>({}, uncoded_size);
}

inline static void mark_buffer_write(stream::RawBuffer<>& buffer,
                                     size_t write_size) {
  buffer.append(write_size);
}

#if defined(H2V_HPACK_HUFFMAN_ENCODER_USE_BIT_OP) || \
    (H2V_HPACK_HUFFMAN_ENCODER_USE_BIT_OP == 1)

/// Huffman Encode using bits operations.
inline static HpackErrorCode FastEncode(const uint8_t* in_ptr, size_t in_size,
                                        uint8_t* out_ptr, size_t out_size,
                                        size_t& encoded_size,
                                        bool trace = false) noexcept {
  if (in_size == 0) {
    encoded_size = 0;
    return HPACK_ERR::NONE;
  }

  if (out_size == 0) {
    encoded_size = 0;
    return HPACK_ERR::BUFFER_TO_SMALL;
  }

  if (!in_ptr) {
    encoded_size = 0;
    return HPACK_ERR::INPUT_NULL_PTR;
  }
  if (!out_ptr) {
    encoded_size = 0;
    return HPACK_ERR::OUTPUT_NULL_PTR;
  }

  uint64_t acc = 0;  // accumulator of bits (left-aligned within 64)
  int bits = 0;      // how many bits are currently in acc
  size_t outpos = 0;

  for (size_t i = 0; i < in_size; i++) {
    uint8_t sym = *(in_ptr + i);       // in[i];
    uint32_t cw = huffman::CODE[sym];  // e.g. 0b10110011… right-justified
    uint8_t clen = huffman::LEN[sym];

    // Left‐align cw into the top ‘clen’ bits: shift code to the leftmost
    // position of a 32-bit word, then shift right so that its MSB is at bit
    // (64-clen). A common trick is: ((uint64_t)cw) << (64 − clen)  if cw was
    // right-aligned.
    acc = (acc << clen) | cw;
    bits += clen;

    // Flush out as many top bytes (or 32 bits) as we can in one shot:
    while (bits >= 32) {
      // We know the top 32 bits of ‘acc’ are a full word ready for output.
      uint32_t word = (uint32_t)(acc >> (bits - 32));

      // Write those 4 bytes (big‐endian) to out_buffer:
      *(out_ptr + outpos) = (uint8_t)(word >> 24);
      *(out_ptr + (outpos + 1)) = (uint8_t)(word >> 16);
      *(out_ptr + (outpos + 2)) = (uint8_t)(word >> 8);
      *(out_ptr + (outpos + 3)) = (uint8_t)(word);

      // outBuf[outpos + 0] = (uint8_t)(word >> 24);
      // outBuf[outpos + 1] = (uint8_t)(word >> 16);
      // outBuf[outpos + 2] = (uint8_t)(word >> 8);
      // outBuf[outpos + 3] = (uint8_t)(word);

      outpos += 4;
      bits -= 32;
      // Mask off the top 32 bits from acc, leaving only the lower ‘bits’ bits:
      acc &= ((uint64_t)1 << bits) - 1;
    }
  }

  // Final flush + padding (pad with 1s up to next byte boundary)
  if (bits > 0) {
    int pad = 8 - (bits & 7);
    // Move whatever is left to the top 32 bits, then pad lowest pad bits
    // with 1.
    uint64_t tmp =
        (acc << (32 - bits)) | (((1ULL << pad) - 1) << (32 - bits - pad));
    // Write top full bytes:
    for (int s = 24; s >= 0; s -= 8) {
      if (bits + pad <= 0)
        break;
      *(out_ptr + (outpos++)) = (uint8_t)(tmp >> s);
      bits -= 8;
    }
  }

  encoded_size = outpos;
  return HPACK_ERR::NONE;
}
#endif

#if !defined(H2V_HPACK_HUFFMAN_ENCODER_USE_BIT_OP) || \
    (H2V_HPACK_HUFFMAN_ENCODER_USE_BIT_OP == 0)

/// Huffman Encode using precomputed lookup table kEncodeTable.
inline static HpackErrorCode FastEncode(const uint8_t* in_ptr, size_t in_size,
                                        uint8_t* out_ptr, size_t out_size,
                                        size_t& encoded_size,
                                        bool trace = false) noexcept {
  if (in_size == 0) {
    encoded_size = 0;
    return HPACK_ERR::NONE;
  }

  if (out_size == 0) {
    encoded_size = 0;
    return HPACK_ERR::BUFFER_TO_SMALL;
  }

  if (in_ptr == nullptr) {
    encoded_size = 0;
    return HPACK_ERR::INPUT_NULL_PTR;
  }
  if (out_ptr == nullptr) {
    encoded_size = 0;
    return HPACK_ERR::OUTPUT_NULL_PTR;
  }

  // acc: lower 'bits_in_acc' bits of 'acc' hold the pending bits
  // (right-justified)
  uint64_t acc = 0;
  int bits_in_acc = 0;
  size_t outpos = 0;

  for (size_t i = 0; i < in_size; i++) {
    uint8_t sym = in_ptr[i];
    const auto& e = table::kEncodeTable[sym];
    int clen = e.bit_length;  // e.g. 6 for 'n'
    int bcnt = e.byte_count;  // e.g. 1 for 'n'

    // 1) Shift existing accumulator left to make room for 'clen' new bits
    acc <<= clen;
    bits_in_acc += clen;

    // 2) OR in the top 'clen' bits from e.bytes[ ]
    //    Reconstruct a 64-bit value whose top 'clen' bits match the code
    //    By design, e.bytes[0..bcnt-1] are the left-aligned code octets.
    uint64_t piece = 0;
    for (int b = 0; b < bcnt; b++) {
      // Place byte[b] into bits 56 - 8*b .. 63 - 8*b of 'piece'
      piece |= (uint64_t(e.bytes[b]) << (56 - 8 * b));
    }

    // Now 'piece' has code bits left-aligned in the top
    // → we want only its top 'clen' bits (i.e. shift right by (64-clen))
    uint64_t code_bits = piece >> (64 - clen);

    acc |= code_bits;

    // 3) Flush out entire 32-bit words whenever bits_in_acc >= 32
    while (bits_in_acc >= 32) {
      int shift = bits_in_acc - 32;
      uint32_t word = uint32_t(acc >> shift);

      // Write those 4 bytes (big-endian)
      out_ptr[outpos++] = uint8_t(word >> 24);
      out_ptr[outpos++] = uint8_t(word >> 16);
      out_ptr[outpos++] = uint8_t(word >> 8);
      out_ptr[outpos++] = uint8_t(word);

      bits_in_acc -= 32;
      if (bits_in_acc > 0) {
        // Mask off the high 32 bits, leaving only the low 'bits_in_acc' bits
        uint64_t mask = ((uint64_t)1 << bits_in_acc) - 1;
        acc &= mask;
      } else {
        acc = 0;
      }
    }
  }

  // Final padding: pad with ones to next byte boundary
  if (bits_in_acc > 0) {
    int pad = (8 - (bits_in_acc & 7)) & 7;  // 1..7

    // Shift left so these remaining bits move to the top of the next byte:
    // Suppose bits_in_acc=11; that means we have 11 leftover bits → pad=5.
    // We want to create a final byte whose top 11 bits are in acc>>
    // (bits_in_acc - 8), but since we flush only by whole bytes, we do:
    //   acc <<= pad;
    //   then OR in (2^pad -1) to set the low 'pad' bits to 1.
    acc <<= pad;
    bits_in_acc += pad;

    // Now the low 'pad' bits should be 1:
    // Example: if pad=5, (1<<5)-1 = 0x1F
    uint64_t fill_mask = ((uint64_t)1 << pad) - 1;
    acc |= fill_mask;

    // We will now flush out any whole bytes (there must be exactly 1..3 bytes).
    while (bits_in_acc >= 8) {
      int shift = bits_in_acc - 8;
      uint8_t final_byte = uint8_t(acc >> shift);

      out_ptr[outpos++] = final_byte;
      bits_in_acc -= 8;
      if (bits_in_acc > 0) {
        uint64_t mask = ((uint64_t)1 << bits_in_acc) - 1;
        acc &= mask;
      } else {
        acc = 0;
      }
    }
  }

  encoded_size = outpos;
  return HPACK_ERR::NONE;
}

#endif

// inline static int32_t FastEncodeFlatmap(const uint8_t* in_ptr, size_t
// in_size,
//                                         uint8_t* out_ptr,
//                                         size_t& out_size) noexcept {
//   if (in_size == 0) {
//     out_size = 0;
//     return HPACK_ERR::NONE;
//   }
//   if (in_ptr == nullptr) {
//     out_size = 0;
//     return HPACK_ERR::INPUT_NULL_PTR;
//   }
//   if (out_ptr == nullptr) {
//     out_size = 0;
//     return HPACK_ERR::OUTPUT_NULL_PTR;
//   }

//   // acc: lower 'bits_in_acc' bits of 'acc' hold the pending bits
//   // (right-justified)
//   uint64_t acc = 0;
//   int bits_in_acc = 0;
//   size_t outpos = 0;

//   std::cout << "[Encode] Starting encoding of " << in_size << " bytes\n";

//   for (size_t i = 0; i < in_size; i++) {
//     uint8_t sym = in_ptr[i];
//     const auto& e = table::kEncodeTable[sym];
//     int clen = e.bit_length;  // e.g. 6 for 'n'
//     int bcnt = e.byte_count;  // e.g. 1 for 'n'
//     std::cout << "--------------------------------------------------\n";
//     std::cout << "[Symbol #" << i << "] byte=0x" << std::hex << int(sym)
//               << std::dec << " ('" << char(sym) << "')"
//               << " → bit_length=" << clen << ", byte_count=" << bcnt
//               << ", raw_bytes={ ";
//     for (int b = 0; b < 5; b++) {
//       std::cout << "0x" << std::hex << int(e.bytes[b]) << std::dec;
//       if (b + 1 < 5)
//         std::cout << ", ";
//     }
//     std::cout << " }\n";

//     // 1) Shift existing accumulator left to make room for 'clen' new bits
//     std::cout << "    [Before shift] acc=0x" << std::hex << std::setw(16)
//               << std::setfill('0') << acc << std::dec
//               << " bits_in_acc=" << bits_in_acc << "\n";
//     acc <<= clen;
//     bits_in_acc += clen;
//     std::cout << "    [After  shift] acc=0x" << std::hex << std::setw(16)
//               << std::setfill('0') << acc << std::dec
//               << " bits_in_acc=" << bits_in_acc << "\n";

//     // 2) OR in the top 'clen' bits from e.bytes[ ]
//     //    Reconstruct a 64-bit value whose top 'clen' bits match the code
//     //    By design, e.bytes[0..bcnt-1] are the left-aligned code octets.
//     uint64_t piece = 0;
//     for (int b = 0; b < bcnt; b++) {
//       // Place byte[b] into bits 56 - 8*b .. 63 - 8*b of 'piece'
//       piece |= (uint64_t(e.bytes[b]) << (56 - 8 * b));
//     }
//     std::cout << "    [Piece from bytes] piece=0x" << std::hex <<
//     std::setw(16)
//               << std::setfill('0') << piece << std::dec << "\n";

//     // Now 'piece' has code bits left-aligned in the top
//     // → we want only its top 'clen' bits (i.e. shift right by (64-clen))
//     uint64_t code_bits = piece >> (64 - clen);
//     std::cout << "    [Extract top bits] code_bits=0x" << std::hex
//               << std::setw(16) << std::setfill('0') << code_bits << std::dec
//               << "\n";

//     acc |= code_bits;
//     std::cout << "    [After OR      ] acc=0x" << std::hex << std::setw(16)
//               << std::setfill('0') << acc << std::dec
//               << " bits_in_acc=" << bits_in_acc << "\n";

//     // 3) Flush out entire 32-bit words whenever bits_in_acc >= 32
//     while (bits_in_acc >= 32) {
//       int shift = bits_in_acc - 32;
//       uint32_t word = uint32_t(acc >> shift);
//       // Write those 4 bytes (big-endian)
//       uint8_t b0 = uint8_t(word >> 24);
//       uint8_t b1 = uint8_t(word >> 16);
//       uint8_t b2 = uint8_t(word >> 8);
//       uint8_t b3 = uint8_t(word);
//       std::cout << "      [Flushing 32 bits] word=0x" << std::hex
//                 << std::setw(8) << std::setfill('0') << word << std::dec
//                 << "\n";
//       std::cout << "        → wrote bytes: { "
//                 << "0x" << std::hex << int(b0) << ", "
//                 << "0x" << std::hex << int(b1) << ", "
//                 << "0x" << std::hex << int(b2) << ", "
//                 << "0x" << std::hex << int(b3) << std::dec << " }\n";

//       out_ptr[outpos++] = b0;
//       out_ptr[outpos++] = b1;
//       out_ptr[outpos++] = b2;
//       out_ptr[outpos++] = b3;

//       bits_in_acc -= 32;
//       if (bits_in_acc > 0) {
//         // Mask off the high 32 bits, leaving only the low 'bits_in_acc' bits
//         uint64_t mask = ((uint64_t)1 << bits_in_acc) - 1;
//         acc &= mask;
//       } else {
//         acc = 0;
//       }
//       std::cout << "      [After flush] acc=0x" << std::hex << std::setw(16)
//                 << std::setfill('0') << acc << std::dec
//                 << " bits_in_acc=" << bits_in_acc << "\n";
//     }
//   }

//   // ─── Final padding: pad with ones to next byte boundary
//   // ─────────────────────
//   if (bits_in_acc > 0) {
//     std::cout << "[Final padding] bits_in_acc=" << bits_in_acc << "\n";
//     int pad = (8 - (bits_in_acc & 7)) & 7;  // 1..7
//     std::cout << "    pad bits = " << pad << "\n";

//     // Shift left so these remaining bits move to the top of the next byte:
//     // Suppose bits_in_acc=11; that means we have 11 leftover bits → pad=5.
//     // We want to create a final byte whose top 11 bits are in acc>>
//     // (bits_in_acc - 8), but since we flush only by whole bytes, we do:
//     //   acc <<= pad;
//     //   then OR in (2^pad -1) to set the low 'pad' bits to 1.
//     acc <<= pad;
//     bits_in_acc += pad;

//     // Now the low 'pad' bits should be 1:
//     // Example: if pad=5, (1<<5)-1 = 0x1F
//     uint64_t fill_mask = ((uint64_t)1 << pad) - 1;
//     acc |= fill_mask;
//     std::cout << "    [After pad OR ] acc=0x" << std::hex << std::setw(16)
//               << std::setfill('0') << acc << std::dec
//               << " bits_in_acc=" << bits_in_acc << "\n";

//     // We will now flush out any whole bytes (there must be exactly 1..3
//     bytes). while (bits_in_acc >= 8) {
//       int shift = bits_in_acc - 8;
//       uint8_t final_byte = uint8_t(acc >> shift);
//       std::cout << "      [Flushing] final byte=0x" << std::hex
//                 << int(final_byte) << std::dec
//                 << " (bits_in_acc=" << bits_in_acc << ")\n";
//       out_ptr[outpos++] = final_byte;
//       bits_in_acc -= 8;
//       if (bits_in_acc > 0) {
//         uint64_t mask = ((uint64_t)1 << bits_in_acc) - 1;
//         acc &= mask;
//       } else {
//         acc = 0;
//       }
//       std::cout << "      [After flush] acc=0x" << std::hex << std::setw(16)
//                 << std::setfill('0') << acc << std::dec
//                 << " bits_in_acc=" << bits_in_acc << "\n";
//     }
//   }

//   out_size = outpos;
//   std::cout << "[Encode] total encoded length = " << out_size << " bytes\n";
//   return HPACK_ERR::NONE;
// }

#if !defined(H2V_HPACK_HUFFMAN_DECODER_USE_FULLBYTE) || \
    (H2V_HPACK_HUFFMAN_DECODER_USE_FULLBYTE == 0)

/// Huffman Decode using 4 Bit Nibble precomputed FSM
inline static HpackErrorCode FastDecode(const uint8_t* in_ptr, size_t in_size,
                                        uint8_t* out_ptr, size_t out_size,
                                        size_t& decoded_size,
                                        bool trace = false) noexcept {
  // We assume it is empty string
  if (in_size == 0) {
    decoded_size = 0;
    return HPACK_ERR::NONE;
  }

  if (out_size < in_size) {
    decoded_size = 0;
    return HPACK_ERR::BUFFER_TO_SMALL;
  }

  if (!in_ptr) {
    decoded_size = 0;
    return HPACK_ERR::INPUT_NULL_PTR;
  }

  if (!out_ptr) {
    decoded_size = 0;
    return HPACK_ERR::OUTPUT_NULL_PTR;
  }

  const uint8_t* ip_end = in_ptr + in_size;

  uint16_t state = 0;  // new‐index in [0..511]
  size_t out_pos = 0;
  while (in_ptr < ip_end) {
    uint8_t b = *in_ptr++;

    // --- high nibble (4 bits)
    {
      uint8_t nib = (b >> 4) & 0x0F;
      size_t idx = static_cast<size_t>(state) * 16 + nib;
      uint32_t packed = huffman::table::kNibbleDecodeTable[idx];
      if ((packed >> 31) & 1) {
        return HPACK_ERR::HUFFMAN_DECODE_INVALID_PREFIX_NIBBLE;
      }
      uint16_t next_state = (packed >> 22) & 0x01FF;  // 9 bits
      uint8_t emit_count = (packed >> 20) & 0x03;     // 2 bits
      if (emit_count == 2) {
        uint8_t c0 = (packed >> 12) & 0xFF;
        uint8_t c1 = (packed >> 4) & 0xFF;
        *(out_ptr + (out_pos)) = c0;
        *(out_ptr + (out_pos + 1)) = c1;
        out_pos += 2;
        // out.push_back(char(c0));
        // out.push_back(char(c1));
      } else if (emit_count == 1) {
        uint8_t c0 = (packed >> 12) & 0xFF;
        *(out_ptr + (out_pos)) = c0;
        out_pos += 1;
        // out.push_back(char(c0));
      }
      state = next_state;
    }

    // --- low nibble (4 bits)
    {
      uint8_t nib = b & 0x0F;
      size_t idx = static_cast<size_t>(state) * 16 + nib;
      uint32_t packed = huffman::table::kNibbleDecodeTable[idx];
      if ((packed >> 31) & 1) {
        return HPACK_ERR::HUFFMAN_DECODE_INVALID_PREFIX_NIBBLE;
      }
      uint16_t next_state = (packed >> 22) & 0x01FF;
      uint8_t emit_count = (packed >> 20) & 0x03;
      if (emit_count == 2) {
        uint8_t c0 = (packed >> 12) & 0xFF;
        uint8_t c1 = (packed >> 4) & 0xFF;
        *(out_ptr + (out_pos)) = c0;
        *(out_ptr + (out_pos + 1)) = c1;
        out_pos += 2;
        // out.push_back(char(c0));
        // out.push_back(char(c1));
      } else if (emit_count == 1) {
        uint8_t c0 = (packed >> 12) & 0xFF;
        *(out_ptr + (out_pos)) = c0;
        out_pos += 1;
        // out.push_back(char(c0));
      }
      state = next_state;
    }
  }

  // padding/EOS check: feed 0..7 bits=1, one bit at a time via
  // kBitTableNibble[state][1],
  // then check the final state’s bit in kAcceptingNibbleBits[].
  auto check_accepting = [&](uint16_t s) -> bool {
    // The “accepting” bitmask was stored 513 bits → 9×64-bit words.
    // If we land on EOS leaf exactly, the old index was 512; here we only
    // packed 512 states (0..511). But when bit-FSM transitions from s lead to
    // EOS, we mark “accepting”. So we just look at that bit.
    size_t w = s / 64;
    size_t b = s % 64;
    return ((huffman::table::kAcceptingNibbleBits[w] >> b) & 1) != 0;
  };

  bool accepted = false;
  for (int pad = 0; pad < 8; ++pad) {
    uint16_t s0 = state;
    bool invalid = false;
    for (int i = 0; i < pad; ++i) {
      auto entry = huffman::table::kBitTableNibble[s0][1];
      if (entry.is_error) {
        invalid = true;
        break;
      }
      s0 = entry.next_state;
    }
    if (!invalid && check_accepting(s0)) {
      accepted = true;
      break;
    }
  }
  if (!accepted) {
    return HPACK_ERR::HUFFMAN_DECODE_INVALID_EOS_PADDING_NIBBLE;
  }

  decoded_size = out_pos;
  return HPACK_ERR::NONE;
}

#endif

#if defined(H2V_HPACK_HUFFMAN_DECODER_USE_FULLBYTE) && \
    (H2V_HPACK_HUFFMAN_DECODER_USE_FULLBYTE == 1)
static HpackErrorCode FastDecode(const uint8_t* in_ptr, size_t in_size,
                                 uint8_t* out_ptr, size_t out_size,
                                 size_t& decoded_size) noexcept;

#endif

}  // namespace huffman
}  // namespace hpack
}  // namespace h2v
