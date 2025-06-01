#include <iostream>

#include "h2v/hpack/huffman_codec.h"
#include "h2v/hpack/utils.h"
#include "h2v/stream/bit_packer.h"

int main() {
  std::cout << "HPACK - Huffman Codec Encoding Test\n";

  h2v::hpack::HuffmanCodec hc;

  std::vector<std::string> inputs = {"https://www.example.com",
                                     "https://api.example.com/v1/trans/trace",
                                     "www.example.com",
                                     "no-cache",
                                     "1337",
                                     "huffman",
                                     "",
                                     "a",
                                     "HPAC",
                                     "hello world hello world hello world"};

  // std::vector<std::string> inputs = {
  //     "no-cache",
  // };

  for (auto& s : inputs) {
    std::cout << "Huffman Encoding for:\n"
              << "----------------------------\n";
    std::cout << s << std::endl;

    std::cout << "\nHuffman::Encode\n";
    auto encode_buff = h2v::hpack::huffman::make_encode_buffer(s.size());
    size_t ec_size = encode_buff.capacity();
    auto encode_err = h2v::hpack::huffman::FastEncodeFlatmap(
        reinterpret_cast<const uint8_t*>(s.data()), s.size(),
        encode_buff.mutable_raw(), ec_size);

    // auto encode_state = hc.Encode(s, result);

    if (encode_err == 0) {
      h2v::hpack::huffman::mark_buffer_write(encode_buff, ec_size);

      std::cout << "Result: OK ✅ [raw:" << s.size() << " encoded:" << ec_size
                << " buffer:" << encode_buff.capacity() << "]\n";
      std::cout << encode_buff.HexDump16(false, true) << "\n";

      std::cout << "Huffman::Decode\n";
      std::string decoded_str;
      auto decode_state = hc.Decode(encode_buff, decoded_str);
      if (decode_state.ok()) {
        std::cout << "Result: OK ✅ ["
                  << "encoded:" << ec_size << " result:" << decoded_str.size()
                  << "]\n"
                  << decoded_str << "\n\n";
      } else {
        std::cout << "Huffman::Decode\n";
        std::cout << "Result: Failed ❌ \n" << std::endl;
      }
    } else {
      std::cout << "Result: Failed ❌ \n" << std::endl;
    }
  }

  return 0;
}