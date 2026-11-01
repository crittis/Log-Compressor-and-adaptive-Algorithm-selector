#ifndef RLE_HPP
#define RLE_HPP

#include <string>

// Compress: output is a sequence of [count][byte] pairs, count in 1..255
std::string rle_compress(const std::string& input);

// Decompress: reverses rle_compress exactly
std::string rle_decompress(const std::string& input);

#endif