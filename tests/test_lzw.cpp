#include <iostream>
#include <string>
#include "lzw.hpp"

static bool roundtrip(const std::string& s, const std::string& name) {
    std::string c = lzw_compress(s);
    std::string d = lzw_decompress(c);
    bool ok = (d == s);
    std::cout << (ok ? "PASS  " : "FAIL  ") << name
              << "  (" << s.size() << " -> " << c.size() << " bytes)\n";
    return ok;
}

int main() {
    bool all = true;
    all &= roundtrip("", "empty string");
    all &= roundtrip("A", "single char");
    all &= roundtrip("ABABABABABA", "repeating pair");
    all &= roundtrip(std::string(1000, 'a'), "long run (tricky decoder case)");
    all &= roundtrip("TOBEORNOTTOBEORTOBEORNOT", "classic LZW example");

    std::string all_bytes;
    for (int i = 0; i < 256; i++) all_bytes.push_back(static_cast<char>(i));
    all &= roundtrip(all_bytes, "all 256 byte values");

    std::string many;
    for (int i = 0; i < 2000; i++)
        many += "2026-11-01 08:00:" + std::to_string(i % 60) + " INFO user" +
                std::to_string(i % 50) + " cache hit\n";
    all &= roundtrip(many, "2000 log-like lines");

    // big pseudo-random-ish text so the 65,536-entry dictionary fills up
    std::string big;
    unsigned x = 12345;
    for (int i = 0; i < 400000; i++) {
        x = x * 1103515245u + 12345u;
        big.push_back(static_cast<char>('a' + (x >> 16) % 6));
    }
    all &= roundtrip(big, "400k chars, dictionary fills up");

    return all ? 0 : 1;
}