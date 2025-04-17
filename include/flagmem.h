#pragma once

#include "addressmappeddevice.h"

#include "nlohmann/json.hpp"

#include <array>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>

#include <cstdint>

template<unsigned int SIZE>
class FlagMem: public AddressMappedDevice
{
public:
    using MemoryArray = std::array<uint8_t, SIZE>;
    using FlagArray = std::array<bool, SIZE>;
private:
    MemoryArray memory;
    FlagArray flags;
public:
    FlagMem() :
        memory{}, flags{}
    {}

    void load_json(const nlohmann::json &json)
    {
        for (const auto &entry : json["ram"])
        {
            uint16_t addr = entry[0].get<uint16_t>();
            uint8_t val = entry[1].get<uint8_t>();
            memory[addr] = val;
        }
    }

    bool verify_state(const nlohmann::json &json)
    {
        for (auto &pair : json["ram"])
        {
            if (memory[pair[0]] != pair[1])
            {
                return false;
            }
        }
        return true;
    }

    void analyse_state(const nlohmann::json &json)
    {
        std::cerr << std::dec;
        for (auto &pair : json["ram"])
        {
            if (memory[pair[0]] != pair[1])
            {
                std::cerr << "Data mismatch at " << pair[0] << ": Expected "
                    << pair[1] << ", got " << (int)memory[pair[0]] << std::endl;
            }
        }
    }

    uint8_t get(uint16_t addr)
    {
        return memory[addr % SIZE];
    }

    void set(uint16_t addr, uint8_t val)
    {
        memory[addr % SIZE] = val;
    }

    bool check(uint16_t addr)
    {
        bool active = flags[addr];
        flags[addr] = false;
        return active;
    }
};