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
    void analyse_state(nlohmann::json json);
    void clock_cycle();
    void attach_bus(Bus *new_bus);
    bool mid_instruction();
private:
    uint8_t last_p; // I hate this

    uint8_t fetch(bool inc);
    
    int ADDR_IMP(); // Implied
    int ADDR_IM(); // Immediate
    int ADDR_ZP(); // Zero-Page
    int ADDR_ZPX(); // Zero-Page X
    int ADDR_AB(); // Absolute
    int ADDR_ABX(bool optimise=true); // Absolute X
    int ADDR_ABY(); // Absolute Y
    int ADDR_INX(); // Index X
    int ADDR_INY(); // Index Y

    uint8_t OP_ORA(int value_idx);
    uint8_t OP_AND(int value_idx);
    uint8_t OP_BRK(int value_idx);
    uint8_t OP_ASL(int value_idx);
    uint8_t OP_EOR(int value_idx);
    uint8_t OP_ROL(int value_idx);
    uint8_t OP_ROR(int value_idx);
    uint8_t OP_DEC(int value_idx);
    uint8_t OP_INC(int value_idx);

    bool WB_ACC(uint8_t val);
    bool WB_ZP(int value_idx, uint8_t val);
    bool WB_NC(int value_idx, uint8_t val, uint8_t inc=0);
    bool WB_CR(int value_idx, uint8_t val, uint8_t inc=0);
};