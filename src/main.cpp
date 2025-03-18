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
            return false;
        }
    }
    
    return true;
}

int main(int argc, char** argv)
{
    const std::string single_step_dir = "external/65x02/nes6502/v1";
    const std::set<std::string> tests_to_run = {
        "ea.json",
        "29.json",
        "25.json",
        "35.json",
        "2d.json"
    };


    fs::directory_iterator dir_iterator{single_step_dir};
    for (const auto &json_file : dir_iterator)
    {
        std::string filename = json_file.path().filename().string();
        if (tests_to_run.find(filename) == tests_to_run.end())
            continue;

        std::ifstream json_stream(json_file.path());
        nlohmann::json tests = nlohmann::json::parse(json_stream);
    
        bool passed = perform_tests(tests);

        std::string opcode_str = fs::path(json_file).stem().string();
        int opcode = std::stoi(opcode_str, nullptr, 16);
        std::cout << "0x" << std::uppercase << std::hex << opcode
            << " : " << (passed ? "PASS" : "FAIL") << std::endl;
    }
}