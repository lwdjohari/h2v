// h2v_huffman_gen_v2.cc
//
// Usage:
//   h2v_huffman_gen --mode=full   output_file.cc
//   h2v_huffman_gen --mode=nibble output_file.cc
//   h2v_huffman_gen --mode=encode output_file.h
// Generates either a full‐byte FSM decoder (512KiB) or a nibble‐based FSM
// decoder (≈16KiB), or the per‐symbol encode table.

#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <queue>
#include <string>
#include <unordered_map>
#include <vector>

#include "h2v/utils/cli/terminal_color.h"
#include "v2/fsm_4bit_nibble_gen.h"
#include "v2/fsm_encode_gen.h"
#include "v2/fsm_full_byte_gen.h"

enum class Mode { Unknown, EmitFullByte, Emit4BitNibble, EmitEncode };
// main(): parse args, build trie, index, then emit tables
//
int main(int argc, char** argv) {
  std::cout << h2v::utils::cli::terminal_color.Blue()
            << "h2v Huffman FSM Table Generator v2.1\n";
  std::cout << "----------------------------------------\n"
            << h2v::utils::cli::terminal_color.Reset();
  if (argc < 2) {
    std::cerr
        << h2v::utils::cli::terminal_color.Green()
        << "Usage: " << h2v::utils::cli::terminal_color.Reset() << argv[0]
        << " --mode=[full|nibble|encode] <output_file.cc|output_file.h>\n";
    return 1;
  }

  Mode mode = Mode::Unknown;
  std::string out_path;
  for (int i = 1; i < argc; ++i) {
    if (std::strcmp(argv[i], "--mode=full") == 0) {
      mode = Mode::EmitFullByte;
    } else if (std::strcmp(argv[i], "--mode=nibble") == 0) {
      mode = Mode::Emit4BitNibble;
    } else if (std::strcmp(argv[i], "--mode=encode") == 0) {
      mode = Mode::EmitEncode;
    } else {
      out_path = argv[i];
    }
  }

  if (mode == Mode::Unknown) {
    std::cerr << h2v::utils::cli::terminal_color.Red()
              << "[ Status ]: " << h2v::utils::cli::terminal_color.Reset()
              << "Codegen mode UNKNOWN, exiting now...\n";
    std::cerr
        << "Usage: " << argv[0]
        << " --mode=[full|nibble|encode] <output_file.cc|output_file.h>\n";
    return 1;
  }

  // if filename empty give default filename
  if (out_path.empty()) {
    if (mode == Mode::Emit4BitNibble) {
      std::cout << h2v::utils::cli::terminal_color.Green()
                << "[ File   ]: " << h2v::utils::cli::terminal_color.Reset()
                << "Default to: "
                   "huffman_byte_table_nibble.h"
                << "\n";
      out_path = "huffman_byte_table_nibble.h";
    } else if (mode == Mode::EmitFullByte) {
      std::cout << h2v::utils::cli::terminal_color.Green()
                << "[ File   ]: " << h2v::utils::cli::terminal_color.Reset()
                << "Default to: "
                   "huffman_byte_table_full.cc"
                << "\n";
      out_path = "huffman_byte_table_full.cc";
    } else {
      std::cout << h2v::utils::cli::terminal_color.Green()
                << "[ File   ]: " << h2v::utils::cli::terminal_color.Reset()
                << "Default to: "
                   "huffman_byte_table_encode.h"
                << "\n";
      out_path = "huffman_byte_table_encode.h";
    }
  } else {
    std::cout << h2v::utils::cli::terminal_color.Green()
              << "[ File   ]: " << h2v::utils::cli::terminal_color.Reset()
              << "Default to: " << out_path << "\n";
  }

  if (mode == Mode::Emit4BitNibble) {
    std::cout << h2v::utils::cli::terminal_color.Green()
              << "[ Mode   ]: " << h2v::utils::cli::terminal_color.Reset()
              << "FSM 4-Bit Nibble table\n";
  } else if (mode == Mode::EmitFullByte) {
    std::cout << h2v::utils::cli::terminal_color.Green()
              << "[ Mode   ]: " << h2v::utils::cli::terminal_color.Reset()
              << "FSM Full-Byte table\n";
  } else {
    std::cout << h2v::utils::cli::terminal_color.Green()
              << "[ Mode   ]: " << h2v::utils::cli::terminal_color.Reset()
              << "FSM Encoded table\n";
  }

  std::cout << h2v::utils::cli::terminal_color.Green()
            << "[ Status ]: " << h2v::utils::cli::terminal_color.Reset()
            << "Generating codegen...\n";

  int result = 0;
  if (mode == Mode::Emit4BitNibble) {
    result = h2v::codegen::huffman_4bit_nibble::GenerateFsm4BitNibble(out_path);
  } else if (mode == Mode::EmitFullByte) {
    result = h2v::codegen::huffman_full_byte::GenerateFsmFullByte(out_path);
  } else {
    result = h2v::codegen::huffman_encode::GenerateEncodeTable(out_path);
  }

  if (result != 0) {
    std::cerr << h2v::utils::cli::terminal_color.Red()
              << "[ Status ]: " << h2v::utils::cli::terminal_color.Reset()
    <<"Codegen failed, exiting now...\n";
    return 1;
  }

  // std::cout << "[ Status ]: Building the Huffman trie...\n";
  // // Build the Huffman trie

  // std::cout << "[ Status ]: Huffman trie finished...\n";
  // // Index all trie nodes via BFS
  // std::cout << "[ Status ]: Indexing nodes...\n";

  // std::cout << "[ Status ]: Indexing nodes finished...\n";

  // Open output file
  std::cout << h2v::utils::cli::terminal_color.Green()
            << "[ Status ]: " << h2v::utils::cli::terminal_color.Reset()
  <<"Codegen output to " << out_path << "...\n\n";

  return 0;
}
