#include "lzw.hpp"
#include <unordered_map>
#include <vector>
#include <cstdint>

namespace {
const uint32_t MAX_CODES = 65536;   // 16-bit codes

void put_u16(std::string& out, uint32_t v) {
    out.push_back(static_cast<char>(v & 0xFF));
    out.push_back(static_cast<char>((v >> 8) & 0xFF));
}
}  // namespace

std::string lzw_compress(const std::string& input) {
    std::string out;

    // dictionary starts with every single byte
    std::unordered_map<std::string, uint32_t> dict;
    for (uint32_t i = 0; i < 256; i++)
        dict[std::string(1, static_cast<char>(i))] = i;
    uint32_t next_code = 256;

    std::string w;   // current phrase
    for (char c : input) {
        std::string wc = w + c;
        if (dict.count(wc)) {
            w = wc;                       // phrase still known: extend it
        } else {
            put_u16(out, dict[w]);        // emit code for the known phrase
            if (next_code < MAX_CODES)
                dict[wc] = next_code++;   // learn the new, longer phrase
            w = std::string(1, c);
        }
    }
    if (!w.empty()) put_u16(out, dict[w]);   // flush the last phrase
    return out;
}

std::string lzw_decompress(const std::string& input) {
    std::string out;
    if (input.size() < 2) return out;

    // decoder rebuilds the same dictionary while it decodes
    std::vector<std::string> dict;
    dict.reserve(MAX_CODES);
    for (uint32_t i = 0; i < 256; i++)
        dict.push_back(std::string(1, static_cast<char>(i)));

    auto read_code = [&](size_t pos) -> uint32_t {
        return static_cast<uint32_t>(static_cast<unsigned char>(input[pos])) |
               (static_cast<uint32_t>(static_cast<unsigned char>(input[pos + 1])) << 8);
    };

    uint32_t code = read_code(0);
    if (code >= dict.size()) return "";      // corrupt input
    std::string prev = dict[code];
    out += prev;

    for (size_t pos = 2; pos + 1 < input.size(); pos += 2) {
        code = read_code(pos);
        std::string entry;
        if (code < dict.size()) {
            entry = dict[code];
        } else if (code == dict.size()) {
            entry = prev + prev[0];          // the tricky "not created yet" case
        } else {
            return "";                       // corrupt input
        }
        out += entry;
        if (dict.size() < MAX_CODES)
            dict.push_back(prev + entry[0]); // same new entry the encoder made
        prev = entry;
    }
    return out;
}