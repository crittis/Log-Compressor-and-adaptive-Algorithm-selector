#include <iostream>
#include <string>
#include "rle.hpp"

int main() {
    std::string sample = "AAAAAAAABBBBCCCD";
    std::string packed = rle_compress(sample);
    std::cout << "Original:   " << sample.size() << " bytes\n";
    std::cout << "Compressed: " << packed.size() << " bytes\n";
    std::cout << "Round trip OK: "
              << (rle_decompress(packed) == sample ? "yes" : "no") << "\n";
    return 0;
}