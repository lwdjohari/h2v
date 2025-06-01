// src/h2v/hpack/huffman_codec.cc

#include "h2v/hpack/huffman_codec.h"

#include "h2v/hpack/error_tracer.h"
#include "h2v/hpack/huffman_table.h"
#include "h2v/hpack/integer_codec.h"
#include "h2v/stream/raw_buffer.h"

// Pull in the generated FSM table:
#include "generated/huffman_byte_table.cc"

namespace h2v {
namespace hpack {

namespace huffman {



}  // namespace huffman

absl::StatusOr<std::size_t> HuffmanCodec::Encode(
    const std::string& input, stream::RawBuffer<>& out) noexcept {
  return Encode(absl::string_view(input.data(), input.size()), out);
}

absl::StatusOr<std::size_t> HuffmanCodec::Encode(
    absl::string_view input, stream::RawBuffer<>& out) noexcept {
  // worst-case: every symbol 30 bits + up to 7 pad bits
  size_t max_bytes = (input.size() * 30 + 7) / 8;
  stream::RawBuffer<> hbuf{/*alloc=*/{}, /*initial_capacity=*/max_bytes};

  if (hbuf.capacity() != max_bytes) {
    return absl::ResourceExhaustedError("Huffman encode allocator failed");
  }

  absl::Span<uint8_t> sptr = hbuf.mutable_data();
  uint8_t* ptr = sptr.data();
  uint64_t bitbuf = 0;
  int bitlen = 0;

  for (unsigned char c : input) {
    uint32_t code = h2v::hpack::huffman::CODE[c];
    uint8_t len = h2v::hpack::huffman::LEN[c];

    // shift in new bits
    bitbuf = (bitbuf << len) | code;
    bitlen += len;

    // --- batch flush here ---
    int full_bytes = bitlen >> 3;  // how many complete bytes
    if (full_bytes) {
      int rem = bitlen & 7;  // bits left after those bytes
      // emit each byte in MSB order:
      //   byte[0] = bits [(bitlen-1) .. (bitlen-8)], etc.
      // unroll up to, say, 4 bytes at once for speed
      for (int i = full_bytes - 1; i >= 0; --i) {
        int shift = rem + (i << 3);
        *ptr++ = uint8_t(bitbuf >> shift);
      }
      // drop the bytes we just wrote
      bitlen = rem;
      if (bitlen)
        bitbuf &= ((uint64_t(1) << bitlen) - 1);
      else
        bitbuf = 0;
    }
  }

  // pad with EOS bits if needed
  if (bitlen > 0) {
    uint32_t eos_code = h2v::hpack::huffman::CODE[256];
    uint8_t eos_len = h2v::hpack::huffman::LEN[256];  // 30
    uint8_t pad = uint8_t(8 - bitlen);
    uint32_t pad_bits = eos_code >> (eos_len - pad);

    bitbuf = (bitbuf << pad) | pad_bits;
    // now exactly one more byte
    *ptr++ = uint8_t(bitbuf & 0xFF);
  }

  // ptr is N past span.data()
  size_t used = ptr - sptr.data();

  // just mark how many bytes written in the end
  hbuf.append(used);
  out = std::move(hbuf);

  return used;
}

absl::StatusOr<size_t> HuffmanCodec::Decode(const uint8_t* ip, size_t ip_size,
                                            std::string& out,
                                            bool trace) noexcept {
  const uint8_t* ipEnd = ip + ip_size;
  out.clear();
  out.reserve(ip_size);  // optional, to reduce reallocations

  uint16_t state = 0;
  while (ip < ipEnd) {
    uint8_t b = *ip++;

    // pointer‐arithmetic lookup into the flat [513×256] table
    const huffman::ByteDecodeEntry* eptr =
        huffman::kByteDecodeTable + (static_cast<size_t>(state) << 8) + b;

    if (trace) {
      size_t key = (state << 8) | b;
      const huffman::ByteDecodeEntry* e = huffman::kByteDecodeTable + key;
      printf("key=%zu state=%u byte=0x%02X emit=%u sym0=0x%02X sym1=0x%02X\n",
             key, state, b, e->emit_count, e->symbols[0], e->symbols[1]);
    }

    switch (eptr->emit_count) {
      case 2:
        out.push_back(
            static_cast<char>(eptr->symbols[0]));  // emit symbol0 first
        out.push_back(static_cast<char>(eptr->symbols[1]));  // then symbol1
        break;
      case 1:
        out.push_back(static_cast<char>(eptr->symbols[0]));
        break;
      default:
        break;
    }

    state = eptr->next_state;
  }

  if (trace) {
    std::cout << "Huffman Decoded before padding: " << out << std::endl;
  }

  // 2) If we landed exactly on the root, no padding bits remain
  if (state == 0) {
    if (trace) {
      std::cout << "[Decode] landed on root, no padding needed\n";
    }
    return out.size();
  }

  // 3) Compute pad_bits from the depth in the last byte
  int depth = huffman::kStateDepth[state] & 7;  // bits consumed in last symbol
  int pad_bits = (8 - depth) & 7;
  if (trace) {
    std::cout << "[Decode] post‐bytes state=" << state << " depth=" << depth
              << " pad_bits=" << pad_bits << "\n";
  }

  // 4) Feed exactly pad_bits of '1'-bits
  for (int i = 0; i < pad_bits; ++i) {
    const auto& be = huffman::kBitTable[state][1];  // transition on bit=1
    if (trace) {
      std::cout << " [Pad] bit#" << i << " state->" << state
                << " emit=" << int(be.emit_count) << " next=" << be.next_state
                << "\n";
    }
    if (be.emit_count != 0) {
      if (trace) {
        std::cout << " [Pad] ❌ emitted symbol during padding\n";
      }
      return absl::InvalidArgumentError(
          "Huffman decode error: padding emitted symbol");
    }
    state = be.next_state;
  }

  // 5) Final accepting-state check
  if (trace) {
    std::cout << "[Decode] after padding state=" << state
              << " accepting=" << (huffman::kAccepting[state] ? "yes" : "no")
              << "\n";
  }
  if (!huffman::kAccepting[state]) {
    if (trace) {
      std::cout << " [Decode] ❌ final state not accepting\n";
    }
    return absl::InvalidArgumentError(
        "Huffman decode error: invalid EOS state");
  }

  if (trace) {
    std::cout << "[Decode] ✅ EOS padding accepted\n";
  }

  return out.size();
}

absl::StatusOr<size_t> HuffmanCodec::Decode(const stream::RawBuffer<>& input,
                                            std::string& out,
                                            bool trace) noexcept {
  return Decode(input.raw(), input.size(), out, trace);
}

}  // namespace hpack
}  // namespace h2v
