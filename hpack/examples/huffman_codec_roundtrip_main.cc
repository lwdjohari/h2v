#include <iostream>

#include "h2v/hpack/huffman_codec.h"
#include "h2v/hpack/utils.h"

int main() {
  std::cout << "HPACK - Huffman Codec Encoding Test\n";

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

    // -----------------------------------------------------------------------------
    // Huffman Encode
    //
    // If compile definition H2V_HPACK_HUFFMAN_ENCODER_USE_BIT_OP = 1
    // FastEncode(...) : use bits operation.
    // If compile definition H2V_HPACK_HUFFMAN_ENCODER_USE_BIT_OP = 0
    // FastEncode(...) : use precomputed lookup table kEncodeTable.
    // -----------------------------------------------------------------------------

    std::cout << "Huffman::Encode\n";
    auto encode_buff = h2v::hpack::huffman::make_encode_buffer(s.size());
    size_t ec_size = encode_buff.capacity();
    auto encode_err = h2v::hpack::huffman::FastEncode(
        reinterpret_cast<const uint8_t*>(s.data()), s.size(),
        encode_buff.mutable_raw(), encode_buff.capacity(), ec_size);

    if (encode_err == h2v::hpack::HPACK_ERR::NONE) {
      // IMPORTANT!
      // Move/Mark the rawbuffer size to the encoded size
      // after getting HPACK_ERR::NONE,
      // We might be have unused bytes because our approach
      // to allocate encoded buffer equal with decoded size()
      // in `huffman::make_enode_buffer(...)`
      h2v::hpack::huffman::mark_buffer_write(encode_buff, ec_size);

      std::cout << "Result: OK ✅ [raw:" << s.size() << " encoded:" << ec_size
                << " buffer:" << encode_buff.capacity() << "]\n";
      std::cout << encode_buff.HexDump16(false, true) << "\n";

      // -----------------------------------------------------------------------------
      // Huffman Decode
      //
      // If compile definition H2V_HPACK_HUFFMAN_DECODER_USE_FULLBYTE = 0
      //  FastDecode(...) : use 4 Bit Nibble precomputed FSM.
      //
      // If compile definition H2V_HPACK_HUFFMAN_DECODER_USE_FULLBYTE = 1
      // FastDecode(...) : use Full-Byte precomputed FSM.
      // -----------------------------------------------------------------------------

      std::cout << "Huffman::Decode\n";

      auto decode_buff =
          h2v::hpack::huffman::make_decode_buffer(encode_buff.size());
      size_t dc_size = decode_buff.capacity();
      auto decode_err = h2v::hpack::huffman::FastDecode(
          encode_buff.raw(), encode_buff.size(), decode_buff.mutable_raw(),
          decode_buff.capacity(), dc_size);

      if (decode_err == h2v::hpack::HPACK_ERR::NONE) {
        // IMPORTANT!
        // Move/Mark the rawbuffer size to the decoded size
        // after getting HPACK_ERR::NONE,
        // We might be have unused bytes because our approach
        // is speculative based on worst case scenario when calculating
        // Decoder Buffer in `huffman::make_decode_buffer(...)`
        h2v::hpack::huffman::mark_buffer_write(decode_buff, dc_size);

        // Convert it to string or even string_view, up to you
        std::string decoded_str =
            std::string(reinterpret_cast<const char*>(decode_buff.raw()),
                        decode_buff.size());

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