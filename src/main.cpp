#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstdio>
#include "rle.hpp"
#include "huffman.hpp"
#include "lzw.hpp"

static std::string read_file(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    std::stringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

// Built-in demo log, used when no file is given
static std::string demo_log() {
    const char* levels[] = {"INFO", "INFO", "INFO", "WARN", "ERROR", "DEBUG"};
    const char* msgs[] = {"user login ok", "user logout", "db query took 12ms",
                          "cache hit", "cache miss", "connection timeout",
                          "file saved", "request received"};
    std::string s;
    for (int i = 0; i < 5000; i++) {
        int sec = i % 60, min = (i / 60) % 60;
        char ts[32];
        snprintf(ts, sizeof(ts), "2026-11-01 08:%02d:%02d", min, sec);
        s += std::string(ts) + " " + levels[i % 6] + " user" +
             std::to_string(i % 50) + " " + msgs[i % 8] + "\n";
    }
    return s;
}

// Shows what we are about to compress: name, size, line count, first lines
static void show_input(const std::string& name, const std::string& data,
                       int preview_lines = 5) {
    size_t lines = 0;
    for (char c : data) if (c == '\n') lines++;

    std::cout << "=========== INPUT ===========\n";
    std::cout << "File:  " << name << "\n";
    std::cout << "Size:  " << data.size() << " bytes\n";
    std::cout << "Lines: " << lines << "\n\n";
    std::cout << "First " << preview_lines << " lines:\n";

    size_t pos = 0;
    for (int i = 0; i < preview_lines && pos < data.size(); i++) {
        size_t end = data.find('\n', pos);
        if (end == std::string::npos) end = data.size();
        std::cout << "  " << data.substr(pos, end - pos) << "\n";
        pos = end + 1;
    }
    std::cout << "=============================\n\n";
}

static void report(const std::string& name, size_t original,
                   const std::string& packed, bool ok) {
    std::cout << name << ":\t" << packed.size() << " bytes  ("
              << 100.0 * packed.size() / original << "% of original)"
              << "  round trip: " << (ok ? "OK" : "FAIL") << "\n";
}

int main(int argc, char** argv) {
    std::string name, data;
    if (argc < 2) {
        name = "(built-in demo log)";
        data = demo_log();
    } else {
        name = argv[1];
        data = read_file(name);
        if (data.empty()) {
            std::cout << "file is empty or missing\n";
            return 1;
        }
    }

    show_input(name, data);

    std::cout << "=========== RESULTS ==========\n";
    std::string r = rle_compress(data);
    report("RLE    ", data.size(), r, rle_decompress(r) == data);

    std::string h = huffman_compress(data);
    report("Huffman", data.size(), h, huffman_decompress(h) == data);

    std::string l = lzw_compress(data);
    report("LZW    ", data.size(), l, lzw_decompress(l) == data);
    return 0;
}