#include <iostream>
#include <string>
#include "rle.hpp"

// Compress then decompress; the result must equal the original.
static bool roundtrip(const std::string& s, const std::string& name) {
    std::string c = rle_compress(s);
    std::string d = rle_decompress(c);
    bool ok = (d == s);
    std::cout << (ok ? "PASS  " : "FAIL  ") << name
              << "  (" << s.size() << " -> " << c.size() << " bytes)\n";
    return ok;
}

int main() {
    bool all = true;
    all &= roundtrip("", "empty string");
    all &= roundtrip("A", "single char");
    all &= roundtrip("AAAABBBC", "simple runs");
    all &= roundtrip("ABCDEFG", "no repeats (expands!)");
    all &= roundtrip("1111222233", "digits");
    all &= roundtrip(std::string(1000, 'x'), "run longer than 255");
    all &= roundtrip("INFO INFO INFO\nINFO INFO\n", "log-like text");
    return all ? 0 : 1;
}