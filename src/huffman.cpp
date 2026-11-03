#include "huffman.hpp"
#include <queue>
#include <vector>
#include <cstdint>
#include <functional>

namespace {

// A tree node. Leaves have left == -1. Nodes live in a vector and refer
// to each other by index, so there are no raw pointers to free.
struct Node {
    unsigned char sym;
    uint32_t freq;
    int left, right;
};

void put_u32(std::string& out, uint32_t v) {
    for (int i = 0; i < 4; i++) out.push_back(static_cast<char>((v >> (8 * i)) & 0xFF));
}

uint32_t get_u32(const std::string& in, size_t& pos) {
    uint32_t v = 0;
    for (int i = 0; i < 4; i++)
        v |= static_cast<uint32_t>(static_cast<unsigned char>(in[pos++])) << (8 * i);
    return v;
}

// Build the Huffman tree from the frequency table; returns the root index.
// Compressor and decompressor both call this with the same table, and the
// order of operations is fixed, so both get an identical tree.
int build_tree(const uint32_t freq[256], std::vector<Node>& pool) {
    using P = std::pair<uint32_t, int>;  // (frequency, node index)
    std::priority_queue<P, std::vector<P>, std::greater<P>> heap;  // min-heap

    for (int s = 0; s < 256; s++) {
        if (freq[s] > 0) {
            pool.push_back({static_cast<unsigned char>(s), freq[s], -1, -1});
            heap.push({freq[s], static_cast<int>(pool.size()) - 1});
        }
    }
    while (heap.size() > 1) {
        P a = heap.top(); heap.pop();   // two smallest
        P b = heap.top(); heap.pop();
        pool.push_back({0, a.first + b.first, a.second, b.second});
        heap.push({a.first + b.first, static_cast<int>(pool.size()) - 1});
    }
    return heap.top().second;
}

// Walk the tree and record each byte's code as a string of '0'/'1'.
void make_codes(const std::vector<Node>& pool, int idx,
                std::string& cur, std::string codes[256]) {
    const Node& n = pool[idx];
    if (n.left == -1) {
        codes[n.sym] = cur.empty() ? "0" : cur;  // only one distinct byte -> code "0"
        return;
    }
    cur.push_back('0'); make_codes(pool, n.left, cur, codes);  cur.pop_back();
    cur.push_back('1'); make_codes(pool, n.right, cur, codes); cur.pop_back();
}

}  // namespace

std::string huffman_compress(const std::string& input) {
    std::string out;
    put_u32(out, static_cast<uint32_t>(input.size()));
    if (input.empty()) return out;

    // 1. count frequencies
    uint32_t freq[256] = {0};
    for (unsigned char c : input) freq[c]++;

    // 2. header: number of distinct bytes (stored as count-1 so 256 fits), then the table
    int distinct = 0;
    for (int s = 0; s < 256; s++) if (freq[s] > 0) distinct++;
    out.push_back(static_cast<char>(distinct - 1));
    for (int s = 0; s < 256; s++) {
        if (freq[s] > 0) {
            out.push_back(static_cast<char>(s));
            put_u32(out, freq[s]);
        }
    }

    // 3. tree and codes
    std::vector<Node> pool;
    int root = build_tree(freq, pool);
    std::string codes[256];
    std::string cur;
    make_codes(pool, root, cur, codes);

    // 4. pack the bits, 8 per output byte (last byte padded with zeros)
    unsigned char buffer = 0;
    int nbits = 0;
    for (unsigned char c : input) {
        for (char bit : codes[c]) {
            buffer = static_cast<unsigned char>((buffer << 1) | (bit == '1' ? 1 : 0));
            if (++nbits == 8) {
                out.push_back(static_cast<char>(buffer));
                buffer = 0;
                nbits = 0;
            }
        }
    }
    if (nbits > 0) {
        buffer = static_cast<unsigned char>(buffer << (8 - nbits));
        out.push_back(static_cast<char>(buffer));
    }
    return out;
}

std::string huffman_decompress(const std::string& input) {
    size_t pos = 0;
    uint32_t n = get_u32(input, pos);       // original length
    if (n == 0) return "";

    int distinct = static_cast<unsigned char>(input[pos++]) + 1;
    uint32_t freq[256] = {0};
    for (int i = 0; i < distinct; i++) {
        unsigned char s = static_cast<unsigned char>(input[pos++]);
        freq[s] = get_u32(input, pos);
    }

    std::vector<Node> pool;
    int root = build_tree(freq, pool);

    std::string out;
    out.reserve(n);

    // special case: only one distinct byte, no bits needed
    if (pool[root].left == -1) {
        out.assign(n, static_cast<char>(pool[root].sym));
        return out;
    }

    // walk the tree bit by bit; on reaching a leaf, emit its byte and restart at root
    int node = root;
    for (; pos < input.size() && out.size() < n; pos++) {
        unsigned char byte = static_cast<unsigned char>(input[pos]);
        for (int b = 7; b >= 0 && out.size() < n; b--) {
            int bit = (byte >> b) & 1;
            node = bit ? pool[node].right : pool[node].left;
            if (pool[node].left == -1) {
                out.push_back(static_cast<char>(pool[node].sym));
                node = root;
            }
        }
    }
    return out;
}