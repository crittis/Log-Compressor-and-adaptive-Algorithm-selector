#include "rle.hpp"
using namespace std;

string rle_compress(const string& input) {
    string out;
    size_t i = 0;

    while (i < input.size()) {
        char current = input[i];
        size_t run = 1;

        // extend the run while the next byte is the same (max 255)
        while (i + run < input.size() && input[i + run] == current && run < 255) {
            run++;
        }

        out.push_back(static_cast<char>(run));  // count byte
        out.push_back(current);                 // the repeated byte
        i += run;
    }
    return out;
}

string rle_decompress(const string& input) {
    string out;

    // read two bytes at a time: [count][byte]
    for (size_t i = 0; i + 1 < input.size(); i += 2) {
        unsigned char count = static_cast<unsigned char>(input[i]);
        char value = input[i + 1];
        out.append(count, value);   // append 'count' copies of value
    }
    return out;
}