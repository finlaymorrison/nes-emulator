#include "ram.h"
#include "cpu.h"
#include "bus.h"

#include "nlohmann/json.hpp"

#include <iostream>
#include <filesystem>
#include <fstream>
#include <string>
#include <set>
#include <iomanip>
#include <sstream>
#include <vector>
#include <algorithm>


namespace fs = std::filesystem;

bool perform_tests(const nlohmann::json &tests)
{
    for (auto &test_case : tests)
    {
        RAM ram;
        CPU cpu;

        cpu.load_json(test_case["initial"]);
        ram.load_json(test_case["initial"]);

        Bus bus(&ram);
        cpu.attach_bus(&bus);

        do
        {
            cpu.clock_cycle();
        }
        while (cpu.mid_instruction());

        if (!cpu.verify_state(test_case["final"]) ||
            !ram.verify_state(test_case["final"]) ||
            !bus.verify_operations(test_case["cycles"]))
        {
            cpu.analyse_state(test_case["final"]);
            ram.analyse_state(test_case["final"]);
            bus.analyse_operations(test_case["cycles"]);
            return false;
        }
    }
    
    return true;
}

// Set of official 6502 opcodes
// const std::set<int> official_opcodes = {
//     0x00, 0x01, 0x05, 0x06, 0x08, 0x09, 0x0A, 0x0D, 0x0E,
//     0x10, 0x11, 0x15, 0x16, 0x18, 0x19, 0x1D, 0x1E,
//     0x20, 0x21, 0x24, 0x25, 0x26, 0x28, 0x29, 0x2A, 0x2C, 0x2D, 0x2E,
//     0x30, 0x31, 0x35, 0x36, 0x38, 0x39, 0x3D, 0x3E,
//     0x40, 0x41, 0x45, 0x46, 0x48, 0x49, 0x4A, 0x4C, 0x4D, 0x4E,
//     0x50, 0x51, 0x55, 0x56, 0x58, 0x59, 0x5D, 0x5E,
//     0x60, 0x61, 0x65, 0x66, 0x68, 0x69, 0x6A, 0x6C, 0x6D, 0x6E,
//     0x70, 0x71, 0x75, 0x76, 0x78, 0x79, 0x7D, 0x7E,
//     0x81, 0x84, 0x85, 0x86, 0x88, 0x8A, 0x8C, 0x8D, 0x8E, 0x90, 0x91,
//     0x94, 0x95, 0x96, 0x98, 0x99, 0x9A, 0x9D,
//     0xA0, 0xA1, 0xA2, 0xA4, 0xA5, 0xA6, 0xA8, 0xA9, 0xAA, 0xAC, 0xAD, 0xAE,
//     0xB0, 0xB1, 0xB4, 0xB5, 0xB6, 0xB8, 0xB9, 0xBA, 0xBC, 0xBD, 0xBE,
//     0xC0, 0xC1, 0xC4, 0xC5, 0xC6, 0xC8, 0xC9, 0xCA, 0xCC, 0xCD, 0xCE,
//     0xD0, 0xD1, 0xD5, 0xD6, 0xD8, 0xD9, 0xDD, 0xDE,
//     0xE0, 0xE1, 0xE4, 0xE5, 0xE6, 0xE8, 0xE9, 0xEA, 0xEC, 0xED, 0xEE,
//     0xF0, 0xF1, 0xF5, 0xF6, 0xF8, 0xF9, 0xFD, 0xFE
// };

const std::set<int> official_opcodes = {
    0x00, 0x01, 0x05, 0x06, 0x09, 0x0A, 0x0D, 0x0E,
    0x11, 0x15, 0x16, 0x19, 0x1D, 0x1E,
    0x21, 0x25, 0x26, 0x29, 0x2A, 0x2D, 0x2E,
    0x31, 0x35, 0x36, 0x39, 0x3D, 0x3E,
    0x41, 0x45, 0x49, 0x4D,
    0x51, 0x55, 0x59, 0x5D
};

int main(int argc, char** argv)
{
    const std::string single_step_dir = "external/65x02/nes6502/v1";

    std::vector<fs::path> json_files;

    for (const auto& entry : fs::directory_iterator(single_step_dir))
    {
        if (entry.path().extension() == ".json") {
            std::string filename = entry.path().stem().string();

            try {
                int opcode = std::stoi(filename, nullptr, 16);
                if (official_opcodes.count(opcode)) {
                    json_files.push_back(entry.path());
                }
            } catch (const std::exception&) {
                std::cerr << "Skipping invalid file: " << filename << std::endl;
            }
        }
    }

    std::sort(json_files.begin(), json_files.end(), [](const fs::path& a, const fs::path& b) {
        return std::stoi(a.stem().string(), nullptr, 16) < std::stoi(b.stem().string(), nullptr, 16);
    });

    for (const auto& json_file : json_files)
    {
        std::ifstream json_stream(json_file);
        nlohmann::json tests = nlohmann::json::parse(json_stream);

        bool passed = perform_tests(tests);

        int opcode = std::stoi(json_file.stem().string(), nullptr, 16);
        std::cout << "0x" << std::uppercase << std::hex
                  << std::setfill('0') << std::setw(2) << opcode
                  << " : " << (passed ? "PASS" : "FAIL") << std::endl;
    }
}
