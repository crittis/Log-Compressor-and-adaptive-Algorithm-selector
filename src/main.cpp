#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <chrono>
#include <cstdio>
#include "rle.hpp"
#include "huffman.hpp"
#include "lzw.hpp"
#include "adaptive.hpp"

using Clock = std::chrono::steady_clock;
using Fn = std::string (*)(const std::string&);

static double ms_since(Clock::time_point t0) {
    return std::chrono::duration<double, std::milli>(Clock::now() - t0).count();
}

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

static void print_table_header() {
    std::printf("%-22s %10s %8s %8s %10s %10s  %s\n",
                "Method", "Size(B)", "Ratio", "Saved", "Comp(ms)", "Decomp(ms)", "OK");
}

static void print_row(const std::string& name, size_t original, size_t packed,
                      double c_ms, double d_ms, bool ok) {
    double ratio = packed ? static_cast<double>(original) / packed : 0.0;
    double saved = 100.0 * (1.0 - static_cast<double>(packed) / original);
    std::printf("%-22s %10zu %7.2fx %7.1f%% %10.2f %10.2f  %s\n",
                name.c_str(), packed, ratio, saved, c_ms, d_ms, ok ? "OK" : "FAIL");
}

// One algorithm on the whole file, with timing
static void run_whole(const std::string& name, Fn comp, Fn decomp, const std::string& data) {
    auto t0 = Clock::now();
    std::string packed = comp(data);
    double c_ms = ms_since(t0);

    t0 = Clock::now();
    std::string back = decomp(packed);
    double d_ms = ms_since(t0);

    print_row(name, data.size(), packed.size(), c_ms, d_ms, back == data);
}

// Adaptive: split into blocks of N lines, best algorithm per block
static void run_adaptive(const std::string& data, size_t lines_per_block) {
    std::vector<BlockInfo> infos;

    auto t0 = Clock::now();
    std::string packed = adaptiveCompress(data, lines_per_block, infos);
    double c_ms = ms_since(t0);

    t0 = Clock::now();
    std::string back = adaptiveDecompress(packed);
    double d_ms = ms_since(t0);
    bool ok = (back == data);

    std::cout << "\n=========== ADAPTIVE (" << lines_per_block << " lines per block, "
              << infos.size() << " blocks) ==========\n";
    std::printf("%-6s %-9s %10s %11s %8s\n", "Block", "Algo", "Original", "Compressed", "Saved");

    const size_t MAX_ROWS = 20;
    size_t used[4] = {0, 0, 0, 0};
    for (size_t i = 0; i < infos.size(); i++) {
        const BlockInfo& b = infos[i];
        used[static_cast<int>(b.algo)]++;
        if (i < MAX_ROWS) {
            double saved = 100.0 * (1.0 - static_cast<double>(b.compressedSize) / b.originalSize);
            std::printf("%-6zu %-9s %10zu %11zu %7.1f%%\n",
                        i, algoName(b.algo), b.originalSize, b.compressedSize, saved);
        }
    }
    if (infos.size() > MAX_ROWS)
        std::cout << "... (" << infos.size() - MAX_ROWS << " more blocks not shown)\n";

    std::cout << "\nAlgorithm chosen:  RAW " << used[0] << "   RLE " << used[1]
              << "   Huffman " << used[2] << "   LZW " << used[3] << "\n\n";
    print_table_header();
    print_row("ADAPTIVE (brute-force)", data.size(), packed.size(), c_ms, d_ms, ok);
    std::cout << "(size includes a 9-byte header per block; compress time includes "
                 "trying every algorithm on every block)\n";
}

int main(int argc, char** argv) {
    // usage: ./bin/logx [file | demo] [lines_per_block]
    std::string name = "demo", data;
    size_t lines_per_block = 500;
    if (argc >= 2) name = argv[1];
    if (argc >= 3) lines_per_block = std::stoul(argv[2]);
    if (lines_per_block == 0) lines_per_block = 500;

    if (name == "demo") {
        name = "(built-in demo log)";
        data = demo_log();
    } else {
        data = read_file(name);
        if (data.empty()) {
            std::cout << "file is empty or missing\n";
            return 1;
        }
    }

    show_input(name, data);

    std::cout << "=========== WHOLE FILE (one algorithm) ==========\n";
    print_table_header();
    run_whole("RLE",     rle_compress,     rle_decompress,     data);
    run_whole("Huffman", huffman_compress, huffman_decompress, data);
    run_whole("LZW",     lzw_compress,     lzw_decompress,     data);

    run_adaptive(data, lines_per_block);
    return 0;
}