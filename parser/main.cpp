#include <iostream>
#include <span>
#include <chrono>
#include <cstdint>

#include <sdsl/int_vector.hpp>

#include "parser.hpp"
#include "random_access_rlz.hpp"

int main(int argc, char* argv[]) {
    if (argc < 4 || argc > 5) {
        std::fprintf(stderr, "usage: %s [reference file] [suffix array file] [input file] {output file}\n", argv[0]);
        std::exit(1);
    }

    auto ref_vec = read_file<std::uint32_t>(argv[1]);
    std::cout << "Read ref" << std::endl;
    auto sa_vec = read_file<std::uint32_t>(argv[2]);
    std::cout << "Read SA" << std::endl;
    auto input_vec = read_file<std::uint32_t>(argv[3]);
    std::cout << "Read DA" << std::endl;

    std::cout << "Starting LZ factoriazation" << std::endl;
    auto start = std::chrono::high_resolution_clock::now();

    auto res = lzFactorize<std::uint32_t, std::uint32_t>(input_vec.data(), input_vec.size(), ref_vec.data(), ref_vec.size(), sa_vec.data());
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration = end - start;

    std::cout << "Finished LZ factoriazation, took: " << duration.count() / 1000 << " seconds" << std::endl;

    if (argc == 5) {
        std::cout << "Will output rlz file" << std::endl;
        start = std::chrono::high_resolution_clock::now();
        std::ofstream ofs(argv[4], std::ios::binary);
        /* for (const auto& [start, pos, len] : res) {
            ofs << "(" << start << ", " << pos << ", " << len << ")\n";
        } */
        // write res to file as binary, this way seems to work
        ofs.write(reinterpret_cast<char*>(res.data()), sizeof(std::size_t) * 3 * res.size());
        /* for (auto [start, pos, len] : res) {
            ofs.write(reinterpret_cast<char*>(&start), sizeof(std::size_t));
            ofs.write(reinterpret_cast<char*>(&pos), sizeof(std::size_t));
            ofs.write(reinterpret_cast<char*>(&len), sizeof(std::size_t));
            // ofs << "(" << start << ", " << pos << ", " << len << ")\n";
        } */
        ofs.close();
        end = std::chrono::high_resolution_clock::now();
        duration = end - start;
        std::cout << "Finished writing file, took: " << duration.count() / 1000 << " seconds" << std::endl;
    }
    
    std::cout << "Building RLZ index" << std::endl;

    start = std::chrono::high_resolution_clock::now();
    random_access_rlz<std::uint32_t> rrlz(ref_vec, res);
    end = std::chrono::high_resolution_clock::now();
    duration = end - start;
    std::cout << "Finished building RLZ index, took: " << duration.count() / 1000 << " seconds" << std::endl;
    const auto [last_start, last_pos, last_len] = res.back();
    const std::size_t decompressed_sz = last_start + last_len;
    const auto avrg_phrase_len = static_cast<double>(decompressed_sz) / static_cast<double>(res.size());

    std::size_t mismatches = 0;
    

    std::cout << "Will count misses" << std::endl;
    start = std::chrono::high_resolution_clock::now();
    sdsl::bit_vector cov_bv(ref_vec.size());
    /* std::cout << "Initialized bitvector of size: " << ref_vec.size() << " res is of size: " << res.size() << " should probably need bits: " << input_vec.size() << std::endl; */
    for (const auto [start, pos, len] : res) {
        if (len == 1) {
            ++mismatches;
        } else {
            if (pos + len >= ref_vec.size()) { 
                std::cout << "pos + i >= ref_vec.size() " << pos << " + " << len << " = " << pos + len << " <= " << ref_vec.size() << " skipping iteration" << std::endl;
                continue;
            }
            
            for (std::size_t i = 0; i < len; ++i) {
                cov_bv[pos + i] = 1;
            }
        }
    }

    std::size_t ones = 0;
    for (const auto& b : cov_bv) {
        ones += b;
    }

    end = std::chrono::high_resolution_clock::now();
    duration = end - start;
    std::cout << "Finished counting misses, took: " << duration.count() / 1000 << " seconds" << std::endl;
    
    const double coverage = static_cast<double>(ones) / static_cast<double>(ref_vec.size());

    std::cout << "size of index: " << rrlz.size_in_bytes() << " bytes" << std::endl;
    std::cout << "size of reference: " << rrlz.ref_vec.size() * sizeof(std::uint32_t) << " bytes\n";
    std::cout << "size of reference pointers: " << rrlz.ref_ptrs.size() * sizeof(std::size_t) << " bytes\n";
    std::cout << "size of starts: " << rrlz.starts.size() / 8 << " bytes\n";
    std::cout << "number of phrases: " << res.size() << "\n";
    std::cout << "average phrase length: " << avrg_phrase_len << "\n";
    std::cout << "length 1 matches: " << mismatches << "\n";
    std::cout << "reference covered: " << coverage << "\n";

    for (std::size_t i = 0; i < input_vec.size(); ++i) {
        if (rrlz.access(i) != input_vec[i]) {
            std::cout << "i: " << i << " | " << rrlz.access(i) << " != " << input_vec[i] << "\n";
        }
    }

}
