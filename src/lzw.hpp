#ifndef LZW_HPP
#define LZW_HPP

#include <string>

std::string lzw_compress(const std::string& input);
std::string lzw_decompress(const std::string& input);

#endif