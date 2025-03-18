#pragma once

#include "nlohmann/json.hpp"

#include <cstdint>
#include <vector>

class Bus;

class CPU
{
private:
    uint16_t pc;
    uint8_t a;
    uint8_t x;
    uint8_t y;
    uint8_t s;
    uint8_t p;

    Bus *bus;
    std::vector<uint8_t> ins_stack;
    int ins_step;
public:
    CPU();
    void load_json(nlohmann::json json);
    bool verify_state(nlohmann::json json);
    void clock_cycle();
    void attach_bus(Bus *new_bus);
    bool mid_instruction();
private:
    uint8_t fetch(bool inc);
    bool OP_NOP_IMP();
    bool OP_AND_IM();
    bool OP_AND_ZP();
    bool OP_AND_ZPX();
    bool OP_AND_AB();
};