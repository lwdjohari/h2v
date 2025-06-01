#include <array>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

namespace h2v {
namespace utils {
namespace byte {  // namespace byte

/// Returns an 8-character string of '0'/'1' bits, MSB first.
inline std::string ToStringBits(uint8_t b) {
  std::string s;
  s.reserve(8);
  for (int i = 7; i >= 0; --i) {
    s.push_back(((b >> i) & 1) ? '1' : '0');
  }
  return s;
}

/// Returns a hex+ASCII dump, grouping bytes into big-endian 16-bit words.
/// Each line contains `words_per_line` words, plus “ | ASCII…” on the right.
inline std::string HexDump16WithASCII(const std::vector<uint8_t>& data,
                                      size_t words_per_line = 8) {
  constexpr char* hex_digits = nullptr;  // silent placeholder
  const size_t bytes_per_line = words_per_line * 2;
  std::ostringstream os;
  os << std::hex << std::setfill('0');

  size_t offset = 0, n = data.size();
  while (offset < n) {
    size_t line_bytes = std::min(bytes_per_line, n - offset);

    // 1) Hex words
    for (size_t i = 0; i < line_bytes; i += 2) {
      uint16_t word = uint16_t(data[offset + i]) << 8;
      if (i + 1 < line_bytes)
        word |= data[offset + i + 1];
      os << std::setw(4) << word;
      if (i + 2 < bytes_per_line)
        os << ' ';
    }

    // pad if last line shorter
    if (line_bytes < bytes_per_line) {
      size_t missing = bytes_per_line - line_bytes;
      // each missing byte → 2 hex + 1 space (except after final word)
      size_t pad_chars = missing * 2 + (missing > 0 ? missing - 1 : 0);
      os << std::string(pad_chars, ' ');
    }

    // 2) ASCII
    os << " | ";
    for (size_t i = 0; i < line_bytes; ++i) {
      char c = data[offset + i];
      os << (std::isprint(static_cast<unsigned char>(c)) ? c : '.');
    }
    os << '\n';

    offset += line_bytes;
  }

  os << std::dec;
  return os.str();
}

/// Returns a classic hex dump with offsets, hex bytes, and ASCII on the right.
/// Each line shows `bytes_per_line` bytes (default 16).
inline std::string HexDump(const std::vector<uint8_t>& data,
                           size_t bytes_per_line = 16) {
  std::ostringstream os;
  os << std::hex << std::setfill('0');

  const size_t n = data.size();
  for (size_t offset = 0; offset < n; offset += bytes_per_line) {
    size_t line_bytes = std::min(bytes_per_line, n - offset);

    // 1) Offset
    os << std::setw(8) << offset << "  ";

    // 2) Hex bytes
    for (size_t i = 0; i < bytes_per_line; ++i) {
      if (i < line_bytes) {
        os << std::setw(2) << static_cast<int>(data[offset + i]);
      } else {
        os << "  ";
      }
      os << ' ';
      if (i == (bytes_per_line / 2 - 1))
        os << ' ';  // extra mid-line space
    }

    // 3) ASCII
    os << " |";
    for (size_t i = 0; i < line_bytes; ++i) {
      unsigned char c = data[offset + i];
      os << (std::isprint(c) ? static_cast<char>(c) : '.');
    }
    os << "|\n";
  }

  os << std::dec;
  return os.str();
}
}  // namespace byte
}  // namespace utils
}  // namespace h2v
