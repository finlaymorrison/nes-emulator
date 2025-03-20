#pragma once

#include "nlohmann/json.hpp"

#include <array>
#include <vector>
#include <string>

#include <cstdint>

constexpr int RAM_SIZE = 0xFFFF;

class RAM
{
public:
    using MemoryArray = std::array<uint8_t, RAM_SIZE>;
private:
    MemoryArray memory;
public:
    RAM();
    void load_json(nlohmann::json json);
    bool verify_state(nlohmann::json json);
    void analyse_state(nlohmann::json json);
    uint8_t get(uint16_t addr);
    void set(uint16_t addr, uint8_t val);
};