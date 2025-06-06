// File: fsm_encode_gen.h
//
// Generates a single header "huffman_encode_table.h" containing:
//
//   1) RFC 7541 CODE[257], LEN[257] arrays (for reference/documentation).
//   2) `struct EncodeEntry`
//   3) `constexpr EncodeEntry kEncodeTable[257]` with each code left‐aligned.
//
// After running, include "huffman_encode_table.h" wherever you need to encode.
//
// Note:  You will need to write your own `FastEncodeFlatmap(...)` in a .h file,
//        referencing `kEncodeTable[sym]` as shown in the comments below.

#pragma once
#include <array>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>

#include "huffman_table.h"

namespace h2v {
namespace codegen {
namespace huffman_encode {

// -----------------------------------------------------------------------------
// Emit the EncodeEntry struct + precomputed table
// -----------------------------------------------------------------------------
inline int GenerateEncodeTable(const std::string& out_path) {
  std::ofstream out(out_path, std::ios::trunc);
  if (!out) {
    std::cerr << "Error: cannot open " << out_path << " for writing\n";
    return 1;
  }

   out << "// @generated by h2v_huffman_gen_v2 utility\n"
         "// DO NOT EDIT THIS FILE DIRECTLY.\n"
         "// IF THIS FILE MISMATCH, HUFFMAN ENCODE WILL FAIL.\n\n"
         "#pragma once\n\n"
         "/* clang-format off*/\n"
         "#include <cstdint>\n\n"
         "namespace h2v { namespace hpack { namespace huffman { namespace table {\n\n"
         "  // Each entry holds:\n"
         "  //   - bit_length (5..30)\n"
         "  //   - byte_count   = (bit_length + 7) / 8\n"
         "  //   - bytes[5]     = Huffman code bits, left-aligned (MSB‐first)\n"
         "  struct EncodeEntry {\n"
         "    uint8_t bit_length;\n"
         "    uint8_t byte_count;\n"
         "    uint8_t bytes[5];\n"
         "  };\n\n"
         "  static constexpr EncodeEntry kEncodeTable[257] = {\n";

  // 3) For each symbol, compute the left-aligned bytes:
  for (int sym = 0; sym < 257; sym++) {
    uint32_t code = h2v::codegen::huffman::CODE[sym];      // LSB-aligned integer
    uint8_t length = h2v::codegen::huffman::LEN[sym];      // bit length

    // Left-shift so that the highest `length` bits of a 64-bit integer hold `code`:
    // e.g. if code=0x2A (<< 0), length=6, then bits64 = 0x2A << (64−6) = 0xA8_0000...00
    uint64_t bits64 = (uint64_t(code) << (64 - length));

    // Compute byte_count = how many full octets are needed to hold `length` bits:
    int byte_count = (length + 7) / 8;  // in [1..5]

    // Break bits64 into five bytes (big-endian), storing only the first `byte_count`.
    uint8_t outBytes[5] = {0, 0, 0, 0, 0};
    for (int b = 0; b < byte_count; b++) {
      // Byte `b` = bits64 >> (56 − 8*b)
      outBytes[b] = uint8_t(bits64 >> (56 - 8 * b));
    }

    // Emit the initializer line:
    out << "    { "
        << int(length) << ", "
        << int(byte_count) << ", { ";
    for (int b = 0; b < 5; b++) {
      out << "0x" << std::hex << int(outBytes[b]) << std::dec;
      if (b + 1 < 5) out << ", ";
    }
    out << " } }";
    if (sym + 1 < 257) out << ",";
    out << "\n";
  }

  out << "  };\n\n"
      << "} } } }  // namespace h2v::hpack::huffman::table\n";
  out << "/* clang-format on*/\n";
  out.close();
  std::cout << "→ Generated “" << out_path << "” successfully.\n";
  return 0;

  std::cout << "→ Generated Huffman encode table to " << out_path << "\n";
  return 0;
}

}  // namespace huffman_encode
}  // namespace codegen
}  // namespace h2v