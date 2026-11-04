#pragma once
#include <string>
#include <vector>

enum class Algo : unsigned char { RAW = 0, RLE = 1, HUFFMAN = 2, LZW = 3 };

struct BlockInfo {
    Algo algo;
    size_t originalSize;
    size_t compressedSize;
};

const char* algoName(Algo a);

// Split text into blocks of N lines each.
std::vector<std::string> splitIntoBlocks(const std::string& text, size_t linesPerBlock);

// For every block: try each algorithm, keep the smallest output.
std::string adaptiveCompress(const std::string& text, size_t linesPerBlock,
                             std::vector<BlockInfo>& infos);

// Read each block header, call the right decompressor, join the blocks.
std::string adaptiveDecompress(const std::string& packed);