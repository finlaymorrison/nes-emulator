#include "ram.h"

#include <iostream>

RAM::RAM() :
    memory{}
{}

void RAM::load_json(nlohmann::json json)
{
    for (const auto &entry : json["ram"])
    {
        uint16_t addr = entry[0].get<uint16_t>();
        uint8_t val = entry[1].get<uint8_t>();
        memory[addr] = val;
    }
}

uint8_t RAM::get(uint16_t addr)
{
    return memory[addr];
}

void RAM::set(uint16_t addr, uint8_t val)
{
    memory[addr] = val;
}

bool RAM::verify_state(nlohmann::json json)
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