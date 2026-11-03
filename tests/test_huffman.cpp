#include <iostream>
#include <string>
#include "huffman.hpp"

static bool roundtrip(const std::string& s, const std::string& name) {
    std::string c = huffman_compress(s);
    std::string d = huffman_decompress(c);
    bool ok = (d == s);
    std::cout << (ok ? "PASS  " : "FAIL  ") << name
              << "  (" << s.size() << " -> " << c.size() << " bytes)\n";
    return ok;
}

int main() {
    bool all = true;
    all &= roundtrip("", "empty string");
    all &= roundtrip("A", "single char");
    all &= roundtrip(std::string(500, 'z'), "one distinct byte, repeated");
    all &= roundtrip("AAAAAABBC", "small skewed text");
    all &= roundtrip("2026-11-01 08:00:02 INFO user17 cache hit\n", "one log line");

    // every possible byte value 0..255, to test the binary / 256-symbol case
    std::string all_bytes;
    for (int i = 0; i < 256; i++) all_bytes.push_back(static_cast<char>(i));
    all &= roundtrip(all_bytes, "all 256 byte values");

    // many log-like lines
    std::string many;
    for (int i = 0; i < 2000; i++)
        many += "2026-11-01 08:00:" + std::to_string(i % 60) + " INFO user" +
                std::to_string(i % 50) + " cache hit\n";
    all &= roundtrip(many, "2000 log-like lines");

    return all ? 0 : 1;
}