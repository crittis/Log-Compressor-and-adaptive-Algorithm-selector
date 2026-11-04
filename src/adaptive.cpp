#include "adaptive.hpp"
#include "rle.hpp"
#include "huffman.hpp"
#include "lzw.hpp"
#include <cstdint>
#include <stdexcept>

// ---- adapters: change these if your function names differ ----
static std::string compressWith(const std::string& b, Algo a) {
    switch (a) {
        case Algo::RLE:     return rle_compress(b);
        case Algo::HUFFMAN: return huffman_compress(b);
        case Algo::LZW:     return lzw_compress(b);
        default:            return b;  // RAW: store unchanged
    }
}

static std::string decompressWith(const std::string& d, Algo a) {
    switch (a) {
        case Algo::RLE:     return rle_decompress(d);
        case Algo::HUFFMAN: return huffman_decompress(d);
        case Algo::LZW:     return lzw_decompress(d);
        default:            return d;
    }
}
// ---------------------------------------------------------------

const char* algoName(Algo a) {
    switch (a) {
        case Algo::RLE:     return "RLE";
        case Algo::HUFFMAN: return "Huffman";
        case Algo::LZW:     return "LZW";
        default:            return "RAW";
    }
}

static void putU32(std::string& s, uint32_t v) {
    for (int i = 0; i < 4; i++) s.push_back((char)((v >> (8 * i)) & 0xFF));
}

static uint32_t getU32(const std::string& s, size_t pos) {
    uint32_t v = 0;
    for (int i = 0; i < 4; i++) v |= (uint32_t)(unsigned char)s[pos + i] << (8 * i);
    return v;
}

std::vector<std::string> splitIntoBlocks(const std::string& text, size_t linesPerBlock) {
    std::vector<std::string> blocks;
    std::string cur;
    size_t lines = 0;
    for (char c : text) {
        cur.push_back(c);
        if (c == '\n' && ++lines == linesPerBlock) {
            blocks.push_back(cur);
            cur.clear();
            lines = 0;
        }
    }
    if (!cur.empty()) blocks.push_back(cur);  // last, shorter block
    return blocks;
}

std::string adaptiveCompress(const std::string& text, size_t linesPerBlock,
                             std::vector<BlockInfo>& infos) {
    static const Algo candidates[] = {Algo::RLE, Algo::HUFFMAN, Algo::LZW};
    std::string packed;

    for (const std::string& block : splitIntoBlocks(text, linesPerBlock)) {
        // start with RAW as the baseline, so a block never grows
        std::string best = block;
        Algo bestAlgo = Algo::RAW;

        for (Algo a : candidates) {
            std::string out = compressWith(block, a);
            if (out.size() < best.size()) {   // smaller output = better ratio
                best = out;
                bestAlgo = a;
            }
        }

        // block header: [algo 1 byte][original size 4][compressed size 4] then data
        packed.push_back((char)bestAlgo);
        putU32(packed, (uint32_t)block.size());
        putU32(packed, (uint32_t)best.size());
        packed += best;

        infos.push_back({bestAlgo, block.size(), best.size()});
    }
    return packed;
}

std::string adaptiveDecompress(const std::string& packed) {
    std::string result;
    size_t pos = 0;
    while (pos < packed.size()) {
        if (pos + 9 > packed.size()) throw std::runtime_error("corrupt header");
        Algo algo = (Algo)(unsigned char)packed[pos];
        uint32_t compSize = getU32(packed, pos + 5);
        pos += 9;
        if (pos + compSize > packed.size()) throw std::runtime_error("corrupt block");
        result += decompressWith(packed.substr(pos, compSize), algo);
        pos += compSize;
    }
    return result;
}