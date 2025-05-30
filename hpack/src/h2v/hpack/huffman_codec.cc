// src/h2v/hpack/huffman_codec.cc

#include "h2v/hpack/huffman_codec.h"

#include "h2v/hpack/error_tracer.h"
#include "h2v/hpack/huffman_table.h"
#include "h2v/hpack/integer_codec.h"
#include "h2v/hpack/raw_buffer.h"

// Pull in the generated FSM table:
#include "huffman_byte_table.cc"

namespace h2v {
namespace hpack {

absl::StatusOr<std::size_t> HuffmanCodec::Encode(const std::string& input,
                                                 RawBuffer<>& out,
                                                 bool use_highway) noexcept {
  return Encode(absl::string_view(input.data(), input.size()), out, false);
}

absl::StatusOr<std::size_t> HuffmanCodec::Encode(
    absl::string_view input, RawBuffer<>& out, bool /*use_highway*/) noexcept {
  // worst-case: every symbol 30 bits + up to 7 pad bits
  size_t max_bytes = (input.size() * 30 + 7) / 8;
  RawBuffer<> hbuf{/*alloc=*/{}, /*initial_capacity=*/max_bytes};

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

  // // 1) Huffman-encode into temp buffer to measure length
  // RawBuffer<> hbuf{/*alloc=*/{}, /*initial_capacity=*/input.size()};
  // hbuf.reserve(input.size());
  // uint64_t bit_buffer = 0;
  // int bit_count = 0;

  // for (unsigned char c : input) {
  //   uint32_t code = huffman::CODE[c];
  //   int len = huffman::LEN[c];
  //   bit_buffer = (bit_buffer << len) | code;
  //   bit_count += len;
  //   while (bit_count >= 8) {
  //     bit_count -= 8;
  //     uint8_t* p = hbuf.append(1);
  //     if (!p) {
  //       if (auto cb = GetErrorCallback()) {
  //         cb(0,
  //            make_error(0x0001,
  //                       static_cast<uint16_t>(HuffmanSubcode::OutputOverflow)),
  //            "HuffmanCodec::Encode: overflow in hbuf");
  //       }
  //       return absl::InternalError("Huffman encode overflow");
  //     }
  //     *p = static_cast<uint8_t>(bit_buffer >> bit_count);
  //     bit_buffer &= ((uint64_t(1) << bit_count) - 1);
  //   }
  // }

  // if (bit_count > 0) {
  //   // final EOS padding (all 1s)
  //   uint8_t pad = static_cast<uint8_t>((bit_buffer << (8 - bit_count)) |
  //                                      ((uint8_t(1) << (8 - bit_count)) -
  //                                      1));
  //   uint8_t* p = hbuf.append(1);
  //   if (!p) {
  //     if (auto cb = GetErrorCallback()) {
  //       cb(0,
  //          make_error(0x0001,
  //                     static_cast<uint16_t>(HuffmanSubcode::OutputOverflow)),
  //          "HuffmanCodec::Encode: pad overflow");
  //     }
  //     return absl::InternalError("Huffman pad overflow");
  //   }
  //   *p = pad;
  // }

  // // 2) Prefix the HPACK string length using a 7-bit integer with H-bit=1
  // std::size_t hlen = hbuf.data().size();
  // // Reserve exact space: 1 byte for prefix + hlen
  // out.reserve(1 + hlen);
  // if (!EncodeInteger<7>(static_cast<uint32_t>(hlen),
  //                       /*prefix_bits=*/0x80, out)) {
  //   if (auto cb = GetErrorCallback()) {
  //     cb(0,
  //        make_error(0x0001,
  //                   static_cast<uint16_t>(HuffmanSubcode::OutputOverflow)),
  //        "HuffmanCodec::Encode: length prefix overflow");
  //   }
  //   return absl::InternalError("Integer encode overflow");
  // }

  // // 3) Append the Huffman-coded bytes in one bulk write
  // const auto& src = hbuf.data();
  // uint8_t* dst = out.append(src.size());
  // if (!dst) {
  //   if (auto cb = GetErrorCallback()) {
  //     cb(0,
  //        make_error(0x0001,
  //                   static_cast<uint16_t>(HuffmanSubcode::OutputOverflow)),
  //        "HuffmanCodec::Encode: data append overflow");
  //   }
  //   return absl::InternalError("Huffman data append overflow");
  // }
  // memcpy(dst, src.data(), src.size());

  // // 4) Return total bytes written
  // return out.data().size();
}

// bool HuffmanCodec::Encode(absl::string_view input, RawBuffer<>& out,
//                           bool use_highway) noexcept {
//   if (input.empty())
//     return true;
//   // 1) Table-driven fast path for all but last
//   size_t last = input.size() - 1;
//   for (size_t i = 0; i < last; ++i) {
//     auto c = static_cast<unsigned char>(input[i]);
//     const auto& e = huffman::kEncodeTable[c];
//     uint8_t* p = out.append(e.byte_count);
//     // if (!p) return overflow_error();
//     if (!p)
//       return false;

//     memcpy(p, e.bytes, e.byte_count);
//   }

//   // 2) Bit-buffer for final symbol + padding
//   unsigned char c = input[last];
//   uint64_t bit_buffer = huffman::CODE[c];  // kCode[c];
//   int bit_count = huffman::LEN[c];         // kLen[c];
//   // Now flush any full bytes in buffer
//   while (bit_count >= 8) {
//     bit_count -= 8;
//     uint8_t* p = out.append(1);
//     // if (!p) return overflow_error();
//     if (!p)
//       return false;
//     *p = static_cast<uint8_t>(bit_buffer >> bit_count);
//     bit_buffer &= ((uint64_t(1) << bit_count) - 1);
//   }
//   // Final EOS padding
//   if (bit_count > 0) {
//     uint8_t pad = static_cast<uint8_t>((bit_buffer << (8 - bit_count)) |
//                                        ((uint8_t(1) << (8 - bit_count)) -
//                                        1));
//     uint8_t* p = out.append(1);
//     // if (!p) return overflow_error();
//     if (!p)
//       return false;
//     *p = pad;
//   }
//   return true;
// }

absl::StatusOr<size_t> HuffmanCodec::Decode(absl::string_view input,
                                            RawBuffer<>& out,
                                            bool use_highway) noexcept {
  if (use_highway) {
    auto hw = highwayDecode(input, out);
    if (hw.ok())
      return *hw;
  }

  size_t out_count = 0;
  uint32_t state = 0;
  const uint8_t* ptr = reinterpret_cast<const uint8_t*>(input.data());
  const uint8_t* end = ptr + input.size();

  while (ptr < end) {
    uint8_t byte = *ptr++;
    const huffman::ByteDecodeEntry& e =
        huffman::kByteDecodeTable[state * 257 + byte];

    // Detect error entry
    if (e.emit_count == 0xFF) {
      if (auto cb = GetErrorCallback()) {
        cb(0,
           make_error(0x0001,
                      static_cast<uint16_t>(HuffmanSubcode::DecodeError)),
           "HuffmanCodec::decode: invalid Huffman sequence");
      }
      return absl::InvalidArgumentError("Huffman invalid code");
    }

    // Emit decoded symbols
    for (int i = 0; i < e.emit_count; ++i) {
      uint8_t* p = out.append(1);
      if (!p) {
        if (auto cb = GetErrorCallback()) {
          cb(0,
             make_error(0x0001,
                        static_cast<uint16_t>(HuffmanSubcode::OutputOverflow)),
             "HuffmanCodec::decode: output overflow");
        }
        return absl::InvalidArgumentError("Huffman output overflow");
      }
      *p = e.symbols[i];
      ++out_count;
    }

    state = e.next_state;
  }

  // Must end in an accepting state
  if (!huffman::kAccepting[state]) {
    if (auto cb = GetErrorCallback()) {
      cb(0,
         make_error(0x0001, static_cast<uint16_t>(HuffmanSubcode::DecodeError)),
         "HuffmanCodec::decode: incomplete code at end");
    }
    return absl::InvalidArgumentError("Huffman decode incomplete");
  }

  return out_count;
}

}  // namespace hpack
}  // namespace h2v
