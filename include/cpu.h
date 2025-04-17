#pragma once

#include "nlohmann/json.hpp"

#include <cstdint>
#include <vector>

class Bus;

struct StatusRegister
{
    union
    {
        uint8_t value;
        struct
        {
            bool carry : 1;
            bool zero : 1;
            bool interrupt : 1;
            bool _unused0 : 1;
            bool brk : 1;
            bool _unused1 : 1;
            bool overflow : 1;
            bool negative : 1;
        } flags;
    };

    uint8_t get_break_val() const
    {
        StatusRegister temp = *this;
        temp.flags.brk = true;
        return temp.value;
    }
};

class CPU
{
private:
    uint16_t pc;
    uint8_t a;
    uint8_t x;
    uint8_t y;
    uint8_t s;
    StatusRegister p;

    Bus *bus;
    int ins_step;

    StatusRegister last_p; // I hate this
    uint8_t opcode;
    uint16_t addr; // Effective address
    uint16_t buf; // Buffer value
    uint8_t val; // Address value
    uint16_t interrupt_vec; // Interupt vector
    int wb_cycle;

    bool rst;
    bool irq;
    bool nmi;
    
public:
    CPU();
    void load_json(nlohmann::json json);
    bool verify_state(nlohmann::json json);
    void analyse_state(nlohmann::json json);
    void clock_cycle();
    void attach_bus(Bus *new_bus);
    bool mid_instruction();

    void trigger_rst();
    void trigger_irq();
    void trigger_nmi();
private:
    uint8_t fetch(bool inc);

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
    void OP_TST();
    void OP_FLG(uint8_t flgval);
    void OP_CMP(uint8_t cmpval);

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