#pragma once

#include "addressmappeddevice.h"

#include "nlohmann/json.hpp"

#include <array>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>

#include <cstdint>

template<int SIZE>
class Mem: public AddressMappedDevice
{
public:
    using MemoryArray = std::array<uint8_t, SIZE>;
private:
    MemoryArray memory;
public:
    Mem() :
        memory{}
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
        return memory[addr];
    }

    void set(uint16_t addr, uint8_t val)
    {
        memory[addr] = val;
    }

    void load_binary_region(const std::string &filepath, std::streamoff offset, std::size_t length)
    {
        if (length > SIZE)
        {
            throw std::runtime_error("Length to read exceeds memory size.");
        }

        std::ifstream file(filepath, std::ios::binary);
        if (!file.is_open())
        {
            throw std::runtime_error("Failed to open binary file: " + filepath);
        }

        file.seekg(offset);
        file.read(reinterpret_cast<char*>(memory.data()), length);
        if (file.gcount() != static_cast<std::streamsize>(length))
        {
            throw std::runtime_error("Failed to load full region into memory.");
        }
    }
};