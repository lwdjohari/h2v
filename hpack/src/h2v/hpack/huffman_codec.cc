// src/h2v/hpack/huffman_codec.cc

#include "h2v/hpack/huffman_codec.h"
#include "h2v/hpack/error_tracer.h"
#include "h2v/stream/raw_buffer.h"

// Pull in the generated FSM table:
#if defined(H2V_HPACK_HUFFMAN_DECODER_USE_FULLBYTE) && \
    (H2V_HPACK_HUFFMAN_DECODER_USE_FULLBYTE == 1)
#include "generated/huffman_byte_table_full.cc"
#endif

namespace h2v {
namespace hpack {
namespace huffman {

#if defined(H2V_HPACK_HUFFMAN_DECODER_USE_FULLBYTE) && \
    (H2V_HPACK_HUFFMAN_DECODER_USE_FULLBYTE == 1)

HpackErrorCode FastDecode(const uint8_t* ip, size_t in_size, uint8_t* out_ptr,
                          size_t out_size, size_t& decoded_size,
                          bool trace = false) noexcept {
  const uint8_t* ipEnd = ip + in_size;

  if (in_size == 0) {
    decoded_size = 0;
    return HPACK_ERR::NONE;
  }

  if (out_size == 0) {
    decoded_size = 0;
    return HPACK_ERR::BUFFER_TO_SMALL;
  }

  if (!ip) {
    decoded_size = 0;
    return HPACK_ERR::INPUT_NULL_PTR;
  }

  if (!out_ptr) {
    decoded_size = 0;
    return HPACK_ERR::OUTPUT_NULL_PTR;
  }

  uint16_t state = 0;
  size_t outpos = 0;
  while (ip < ipEnd) {
    uint8_t b = *ip++;

    // pointer‐arithmetic lookup into the flat [513×256] table
    const huffman::table::ByteDecodeEntry* eptr =
        huffman::table::kByteDecodeTable + (static_cast<size_t>(state) << 8) + b;

    if (trace) {
      size_t key = (state << 8) | b;
      const huffman::table::ByteDecodeEntry* e = huffman::table::kByteDecodeTable + key;
      printf("key=%zu state=%u byte=0x%02X emit=%u sym0=0x%02X sym1=0x%02X\n",
             key, state, b, e->emit_count, e->symbols[0], e->symbols[1]);
    }

    switch (eptr->emit_count) {
      case 2:
        out_ptr[outpos] = eptr->symbols[0];      // emit symbol0 first
        out_ptr[outpos + 1] = eptr->symbols[1];  // emit symbol1
        outpos += 2;
        break;
      case 1:
        out_ptr[outpos] = eptr->symbols[0];  // emit symbol0 first
        outpos++;
        break;
      default:
        break;
    }

    state = eptr->next_state;
  }

  // 2) If we landed exactly on the root, no padding bits remain
  if (state == 0) {
    if (trace) {
      std::cout << "[Decode] landed on root, no padding needed\n";
    }
    decoded_size = outpos;
    return HPACK_ERR::NONE;
  }

  // 3) Compute pad_bits from the depth in the last byte
  int depth = huffman::table::kStateDepth[state] & 7;  // bits consumed in last symbol
  int pad_bits = (8 - depth) & 7;
  if (trace) {
    std::cout << "[Decode] post‐bytes state=" << state << " depth=" << depth
              << " pad_bits=" << pad_bits << "\n";
  }

  // 4) Feed exactly pad_bits of '1'-bits
  for (int i = 0; i < pad_bits; ++i) {
    const auto& be = huffman::table::kBitTable[state][1];  // transition on bit=1
    if (trace) {
      std::cout << " [Pad] bit#" << i << " state->" << state
                << " emit=" << int(be.emit_count) << " next=" << be.next_state
                << "\n";
    }
    if (be.emit_count != 0) {
      if (trace) {
        std::cout << " [Pad] ❌ emitted symbol during padding\n";
      }
      decoded_size = outpos;
      return HPACK_ERR::HPACK_HUFFMAN_DECODE_PAD_INVALID;
    }
    state = be.next_state;
  }

  // 5) Final accepting-state check
  if (trace) {
    std::cout << "[Decode] after padding state=" << state
              << " accepting=" << (huffman::table::kAccepting[state] ? "yes" : "no")
              << "\n";
  }
  if (!huffman::kAccepting[state]) {
    if (trace) {
      std::cout << " [Decode] ❌ final state not accepting\n";
    }
    decoded_size = outpos;
    return HPACK_ERR::HPACK_HUFFMAN_DECODE_INVALID_EOS;
  }

  if (trace) {
    std::cout << "[Decode] ✅ EOS padding accepted\n";
  }

  decoded_size = outpos;
  return HPACK_ERR::NONE;
}
#endif

}  // namespace huffman

}  // namespace hpack
}  // namespace h2v
