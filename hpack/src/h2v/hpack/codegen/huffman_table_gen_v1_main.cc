#include <array>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <queue>
#include <string>
#include <unordered_map>
#include <vector>

// 1) RFC7541 Appendix B tables Appendix B.  Huffman Code:
static const std::array<uint32_t, 257> CODE = {
    {/*0x1ff8, 0x7fffd8,  … 254 more codewords …  0x3fffffff*/
     0x1ff8,    0x7fffd8,   0xfffffe2, 0xfffffe3, 0xfffffe4,  0xfffffe5,
     0xfffffe6, 0xfffffe7,  0xfffffe8, 0xffffea,  0x3ffffffc, 0xfffffe9,
     0xfffffea, 0x3ffffffd, 0xfffffeb, 0xfffffec, 0xfffffed,  0xfffffee,
     0xfffffef, 0xffffff0,  0xffffff1, 0xffffff2, 0x3ffffffe, 0xffffff3,
     0xffffff4, 0xffffff5,  0xffffff6, 0xffffff7, 0xffffff8,  0xffffff9,
     0xffffffa, 0xffffffb,  0x14,      0x3f8,     0x3f9,      0xffa,
     0x1ff9,    0x15,       0xf8,      0x7fa,     0x3fa,      0x3fb,
     0xf9,      0x7fb,      0xfa,      0x16,      0x17,       0x18,
     0x0,       0x1,        0x2,       0x19,      0x1a,       0x1b,
     0x1c,      0x1d,       0x1e,      0x1f,      0x5c,       0xfb,
     0x7ffc,    0x20,       0xffb,     0x3fc,     0x1ffa,     0x21,
     0x5d,      0x5e,       0x5f,      0x60,      0x61,       0x62,
     0x63,      0x64,       0x65,      0x66,      0x67,       0x68,
     0x69,      0x6a,       0x6b,      0x6c,      0x6d,       0x6e,
     0x6f,      0x70,       0x71,      0x72,      0xfc,       0x73,
     0xfd,      0x1ffb,     0x7fff0,   0x1ffc,    0x3ffc,     0x22,
     0x7ffd,    0x3,        0x23,      0x4,       0x24,       0x5,
     0x25,      0x26,       0x27,      0x6,       0x74,       0x75,
     0x28,      0x29,       0x2a,      0x7,       0x2b,       0x76,
     0x2c,      0x8,        0x9,       0x2d,      0x77,       0x78,
     0x79,      0x7a,       0x7b,      0x7ffe,    0x7fc,      0x3ffd,
     0x1ffd,    0xffffffc,  0xfffe6,   0x3fffd2,  0xfffe7,    0xfffe8,
     0x3fffd3,  0x3fffd4,   0x3fffd5,  0x7fffd9,  0x3fffd6,   0x7fffda,
     0x7fffdb,  0x7fffdc,   0x7fffdd,  0x7fffde,  0xffffeb,   0x7fffdf,
     0xffffec,  0xffffed,   0x3fffd7,  0x7fffe0,  0xffffee,   0x7fffe1,
     0x7fffe2,  0x7fffe3,   0x7fffe4,  0x1fffdc,  0x3fffd8,   0x7fffe5,
     0x3fffd9,  0x7fffe6,   0x7fffe7,  0xffffef,  0x3fffda,   0x1fffdd,
     0xfffe9,   0x3fffdb,   0x3fffdc,  0x7fffe8,  0x7fffe9,   0x1fffde,
     0x7fffea,  0x3fffdd,   0x3fffde,  0xfffff0,  0x1fffdf,   0x3fffdf,
     0x7fffeb,  0x7fffec,   0x1fffe0,  0x1fffe1,  0x3fffe0,   0x1fffe2,
     0x7fffed,  0x3fffe1,   0x7fffee,  0x7fffef,  0xfffea,    0x3fffe2,
     0x3fffe3,  0x3fffe4,   0x7ffff0,  0x3fffe5,  0x3fffe6,   0x7ffff1,
     0x3ffffe0, 0x3ffffe1,  0xfffeb,   0x7fff1,   0x3fffe7,   0x7ffff2,
     0x3fffe8,  0x1ffffec,  0x3ffffe2, 0x3ffffe3, 0x3ffffe4,  0x7ffffde,
     0x7ffffdf, 0x3ffffe5,  0xfffff1,  0x1ffffed, 0x7fff2,    0x1fffe3,
     0x3ffffe6, 0x7ffffe0,  0x7ffffe1, 0x3ffffe7, 0x7ffffe2,  0xfffff2,
     0x1fffe4,  0x1fffe5,   0x3ffffe8, 0x3ffffe9, 0xffffffd,  0x7ffffe3,
     0x7ffffe4, 0x7ffffe5,  0xfffec,   0xfffff3,  0xfffed,    0x1fffe6,
     0x3fffe9,  0x1fffe7,   0x1fffe8,  0x7ffff3,  0x3fffea,   0x3fffeb,
     0x1ffffee, 0x1ffffef,  0xfffff4,  0xfffff5,  0x3ffffea,  0x7ffff4,
     0x3ffffeb, 0x7ffffe6,  0x3ffffec, 0x3ffffed, 0x7ffffe7,  0x7ffffe8,
     0x7ffffe9, 0x7ffffea,  0x7ffffeb, 0xffffffe, 0x7ffffec,  0x7ffffed,
     0x7ffffee, 0x7ffffef,  0x7fffff0, 0x3ffffee, 0x3fffffff}};

static const std::array<uint8_t, 257> LEN = {
    {/*13, 23,  … 254 more lengths …  30*/
     13, 23, 28, 28, 28, 28, 28, 28, 28, 24, 30, 28, 28, 30, 28, 28, 28, 28, 28,
     28, 28, 28, 30, 28, 28, 28, 28, 28, 28, 28, 28, 28, 6,  10, 10, 12, 13, 6,
     8,  11, 10, 10, 8,  11, 8,  6,  6,  6,  5,  5,  5,  6,  6,  6,  6,  6,  6,
     6,  7,  8,  15, 6,  12, 10, 13, 6,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,
     7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  8,  7,  8,  13, 19, 13, 14,
     6,  15, 5,  6,  5,  6,  5,  6,  6,  6,  5,  7,  7,  6,  6,  6,  5,  6,  7,
     6,  5,  5,  6,  7,  7,  7,  7,  7,  15, 11, 14, 13, 28, 20, 22, 20, 20, 22,
     22, 22, 23, 22, 23, 23, 23, 23, 23, 24, 23, 24, 24, 22, 23, 24, 23, 23, 23,
     23, 21, 22, 23, 22, 23, 23, 24, 22, 21, 20, 22, 22, 23, 23, 21, 23, 22, 22,
     24, 21, 22, 23, 23, 21, 21, 22, 21, 23, 22, 23, 23, 20, 22, 22, 22, 23, 22,
     22, 23, 26, 26, 20, 19, 22, 23, 22, 25, 26, 26, 26, 27, 27, 26, 24, 25, 19,
     21, 26, 27, 27, 26, 27, 24, 21, 21, 26, 26, 28, 27, 27, 27, 20, 24, 20, 21,
     22, 21, 21, 23, 22, 22, 25, 25, 24, 24, 26, 23, 26, 27, 26, 26, 27, 27, 27,
     27, 27, 28, 27, 27, 27, 27, 27, 26, 30}};

struct Node {
  Node* child[2] = {nullptr, nullptr};
  int symbol = -1;  // 0–255: octet value; 256: EOS symbol; -1 for internal node
};

int main(int argc, char** argv) {
  // Output path
  std::string out_path = "huffman_byte_table.cc";
  if (argc > 1)
    out_path = argv[1];

  std::ofstream out(out_path, std::ios::trunc);
  if (!out) {
    std::cerr << "Error: cannot open " << out_path << "\n";
    return 1;
  }

  // Build Huffman trie (256 symbols)
  Node* root = new Node();
  for (int sym = 0; sym < 257; ++sym) {
    uint32_t code = CODE[sym];
    uint8_t length = LEN[sym];
    Node* n = root;
    for (int i = length - 1; i >= 0; --i) {
      int bit = (code >> i) & 1;
      if (!n->child[bit])
        n->child[bit] = new Node();
      n = n->child[bit];
    }
    n->symbol = sym;
  }

  // Index each trie node via BFS
  std::vector<Node*> nodes;
  nodes.reserve(600);
  std::unordered_map<Node*, int> index;
  std::queue<Node*> q;
  q.push(root);
  index[root] = 0;
  nodes.push_back(root);

  while (!q.empty()) {
    Node* cur = q.front();
    q.pop();
    for (int b = 0; b < 2; ++b) {
      Node* c = cur->child[b];
      if (c && index.find(c) == index.end()) {
        int idx = nodes.size();
        index[c] = idx;
        nodes.push_back(c);
        q.push(c);
      }
    }
  }

  // Emit C++ table header
  out << "// @generated by h2v_huffman_gen\n"
         "/* clang-format off */\n"
         "#include <cstdint>\n\n"
         "namespace h2v{\n"
         "namespace hpack{\n"
         "namespace huffman{\n"
         "struct ByteDecodeEntry { \n"
         "  uint16_t next_state;\n"
         "  uint8_t emit_count;\n"
         "  uint8_t symbols[2];\n };\n";

  size_t total = nodes.size() * 256;
  out << "inline constexpr ByteDecodeEntry kByteDecodeTable[" << total
      << "] = {\n";

  // ----------------------------------------------------------------------
  // 2) Compute Emit each decoded (state, byte) entry
  // ----------------------------------------------------------------------
  for (size_t st = 0; st < nodes.size(); ++st) {
    for (int byte = 0; byte < 256; ++byte) {
      Node* curr = nodes[st];
      Node* n = curr;
      std::vector<uint8_t> emits;
      bool error = false;

      // Walk 8 bits
      for (int i = 7; i >= 0; --i) {
        int bit = (byte >> i) & 1;
        n = n->child[bit];
        if (!n) {
          error = true;
          break;
        }
        if (n->symbol >= 0) {
          emits.push_back(static_cast<uint8_t>(n->symbol));
          n = root;
        }
      }

      if (error) {
        out << "  { 0, 0xFF, {0,0} },\n";
      } else {
        int cnt = emits.size();
        uint8_t s0 = cnt > 0 ? emits[0] : 0;
        uint8_t s1 = cnt > 1 ? emits[1] : 0;
        int next = index[n];
        out << "  { " << next << ", " << cnt << ", {" << int(s0) << ","
            << int(s1) << "} },\n";
      }
    }
  }

  out << "};\n\n";

  // This is too naive
  // Emit accepting‐state flags (root OR EOS leaf)
  // out << "inline constexpr bool kAccepting[" << nodes.size() << "] = {\n";
  // for (size_t i = 0; i < nodes.size(); ++i) {
  //   bool is_accept = (nodes[i] == root) || (nodes[i]->symbol == 256);
  //   out << (is_accept ? "  true" : "  false");
  //   if (i + 1 < nodes.size())
  //     out << ",";
  //   out << "\n";
  // }
  // out << "};\n";

  // ----------------------------------------------------------------------
  // 3) Compute Emit accepting‐state flags via in-memory trie simulation
  // ----------------------------------------------------------------------
  out << "inline constexpr bool kAccepting[" << nodes.size() << "] = {\n";
  for (size_t i = 0; i < nodes.size(); ++i) {
    bool valid = false;
    Node* cur = nodes[i];
    // Try feeding up to 32 one-bits
    for (int step = 0; step < 32; ++step) {
      Node* next = cur->child[1];
      if (!next) {
        // invalid transition → padding here would emit error
        break;
      }
      if (next->symbol >= 0) {
        // hitting a symbol resets to root
        if (next->symbol == 256) {
          valid = true;  // reached EOS leaf
        }
        cur = root;
      } else {
        cur = next;
      }
      if (valid)
        break;
    }
    out << (valid ? "  true" : "  false");
    if (i + 1 < nodes.size())
      out << ", ";
    out << "\n";
  }
  out << "};\n\n";

  // ----------------------------------------------------------------------
  // 4) Compute bit-depth for each state (distance from last symbol emission)
  // ----------------------------------------------------------------------
  std::vector<uint8_t> stateDepth(nodes.size());
  // We know root (state 0) is depth 0
  stateDepth[0] = 0;
  // Use a queue for BFS
  std::queue<Node*> dq;
  dq.push(root);
  std::vector<bool> seen(nodes.size());
  seen[0] = true;
  while (!dq.empty()) {
    Node* cur = dq.front();
    dq.pop();
    int idx = index[cur];
    for (int bit = 0; bit < 2; ++bit) {
      Node* c = cur->child[bit];
      if (c) {
        int ci = index[c];
        if (!seen[ci]) {
          // each child is one bit further from its parent
          stateDepth[ci] = uint8_t(stateDepth[idx] + 1);
          seen[ci] = true;
          dq.push(c);
        }
      }
    }
  }

  // ----------------------------------------------------------------------
  // 4) Compute & Emit the depth table
  //    State bit-depth: how many bits into current codeword
  // ----------------------------------------------------------------------
  out << "// State bit-depth: how many bits into current codeword\n";
  out << "inline constexpr uint8_t kStateDepth[" << nodes.size() << "] = {";
  for (size_t i = 0; i < stateDepth.size(); ++i) {
    out << int(stateDepth[i]);
    if (i + 1 < stateDepth.size())
      out << ", ";
  }
  out << "};\n\n";

  // ------------------------------------------------------------------------
  // 5) Emit the bit-level transition table:
  //
  //    For each FSM state and each possible single bit (0 or 1), we need
  //    to know (a) what next_state we’d end up in, and (b) how many symbols
  //    (0 or 1) that bit would emit.  In HPACK, feeding a padding bit '1'
  //    must never emit a symbol.
  // ------------------------------------------------------------------------
  out << "struct BitDecodeEntry {\n"
         "  uint16_t next_state;\n"
         "  uint8_t  emit_count;\n"
         "};\n\n"
         "inline constexpr BitDecodeEntry kBitTable["
      << nodes.size() << "][2] = {\n";

  // We already have `nodes` and the same trie used for bytes.  We just walk
  // one bit at a time from each state:
  for (size_t st = 0; st < nodes.size(); ++st) {
    out << "  { ";
    for (int bit = 0; bit < 2; ++bit) {
      Node* n = nodes[st];
      // follow a single bit
      if (n->child[bit]) {
        n = n->child[bit];
      } else {
        // invalid transition: mark as error state (never used in padding)
        out << "{ 0xFFFF, 0xFF }";
        if (bit == 0)
          out << ", ";
        continue;
      }

      // if we landed on a symbol, that would be an emit_count=1 and we reset
      // to root; otherwise emit_count=0 and stay in this node.
      // bool is_leaf = (n->symbol >= 0);
      // uint16_t next = is_leaf ? /*root=*/0 : static_cast<uint16_t>(st);
      bool is_leaf = (n->symbol >= 0);
      // child ‘n’ gives next_state = its index(), unless it’s a leaf (reset to
      // root)
      uint16_t next = is_leaf ? /*root=*/0 : static_cast<uint16_t>(index[n]);

      uint8_t cnt = is_leaf ? 1 : 0;
      out << "{ " << next << ", " << int(cnt) << " }";
      if (bit == 0)
        out << ", ";
    }
    out << " },\n";
  }
  out << "};\n\n";

  // ----------------------------------------------------------------------
  // 6) Write namespace closure
  // ----------------------------------------------------------------------
  out << "}\n}\n}\n"
      << "/* clang-format off */";

  std::cout << "Generated Huffman FSM with " << nodes.size() << " states to "
            << out_path << "\n";
  return 0;
}
