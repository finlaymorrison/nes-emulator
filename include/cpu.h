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
    uint8_t opcode;
    uint16_t addr; // Effective address
    uint16_t buf; // Buffer value
    uint8_t val; // Address value
    int wb_cycle;

    uint8_t fetch(bool inc);
    
    void update_flag(uint8_t flag, bool condition);
    void set_flag(uint8_t flag);
    void clear_flag(uint8_t flag);

    bool ADDR_IMP(); // Implied
    bool ADDR_IM(); // Immediate
    bool ADDR_ZP(); // Zero-Page
    bool ADDR_AB(); // Absolute
    bool ADDR_INX(); // Index X
    bool ADDR_INY(); // Index Y
    bool ADDR_REL(); // PC Relative
    bool zeropage_indexed(uint8_t index, bool resolve=true);
    bool absolute_indexed(uint8_t index, bool resolve=true, bool optimise=true);

    bool ADDR_ZP_R();
    bool ADDR_AB_R();
    bool ADDR_ABP_R();
    bool ADDR_IN_R();
    bool ADDR_INX_R();
    bool ADDR_INY_R(bool optimise=true);

    uint8_t OP_ADC();
    uint8_t OP_SBC();
    uint8_t OP_ORA();
    uint8_t OP_AND();
    uint8_t OP_ASL();
    uint8_t OP_LSR();
    uint8_t OP_EOR();
    uint8_t OP_ROL();
    uint8_t OP_ROR();
    uint8_t OP_DEC();
    uint8_t OP_INC();
    uint8_t OP_TST();
    void OP_FLG(uint8_t flgval);
    uint8_t OP_CMP(uint8_t cmpval);

    bool WB_MEM(uint8_t comp_val, bool delay=true);
    
    bool WB_BRK();
    bool WB_PHP();
    bool WB_PHA();
    bool WB_PLA();
    bool WB_PLP();
    bool WB_JSR();
    bool WB_RTI();
    bool WB_RTS();

    bool BRANCH();
};