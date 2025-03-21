#include "cpu.h"
#include "bus.h"

#include <iostream>
#include <iomanip>

#define FLG_NEG 0b10000000
#define FLG_OVR 0b01000000
#define FLG_BRK 0b00010000
#define FLG_INT 0b00000100
#define FLG_ZRO 0b00000010
#define FLG_CRY 0b00000001

CPU::CPU() :
    pc{}, a{}, x{}, y{}, s{}, p{}, bus{nullptr}, ins_stack{}, ins_step{0}, last_p{}
{
    ins_stack.reserve(10);
}

void CPU::load_json(nlohmann::json json)
{
    pc = json["pc"].get<uint16_t>();
    a = json["a"].get<uint8_t>();
    x = json["x"].get<uint8_t>();
    y = json["y"].get<uint8_t>();
    s = json["s"].get<uint8_t>();
    p = json["p"].get<uint8_t>();
}

void CPU::attach_bus(Bus *new_bus)
{
    bus = new_bus;
}

bool CPU::mid_instruction()
{
    return ins_step != 0;
}

bool CPU::verify_state(nlohmann::json json)
{
    return json["pc"] == pc &&
           json["a"] == a &&
           json["x"] == x &&
           json["y"] == y &&
           json["s"] == s &&
           json["p"] == p;
}

void CPU::analyse_state(nlohmann::json json)
{
    std::cerr << std::dec;
    if (json["pc"] != pc) {
        std::cerr << "State mismatch in PC: Expected " << json["pc"] << ", got " << (int)pc << std::endl;
    }

    if (json["a"] != a) {
        std::cerr << "State mismatch in A: Expected " << json["a"] << ", got " << (int)a << std::endl;
    }

    if (json["x"] != x) {
        std::cerr << "State mismatch in X: Expected " << json["x"] << ", got " << (int)x << std::endl;
    }

    if (json["y"] != y) {
        std::cerr << "State mismatch in Y: Expected " << json["y"] << ", got " << (int)y << std::endl;
    }

    if (json["s"] != s) {
        std::cerr << "State mismatch in S: Expected " << json["s"] << ", got " << (int)s << std::endl;
    }

    if (json["p"] != p) {
        std::cerr << "State mismatch in P: Expected " << json["p"] << ", got " << (int)p << std::endl;
    }
}

uint8_t CPU::fetch(bool inc)
{
    uint8_t byte = bus->get(pc);
    if (inc) ++pc;
    return byte;
}

bool CPU::WB_ACC(uint8_t val)
{
    a = val;
    return true;
}

bool CPU::WB_X(uint8_t val)
{
    x = val;
    return true;
}

bool CPU::WB_Y(uint8_t val)
{
    y = val;
    return true;
}

bool CPU::WB_ZP(int value_idx, uint8_t val)
{
    if (ins_step < ins_stack.size())
    {
        bus->set(ins_stack[value_idx-1], ins_stack[value_idx]);
        return false;
    }
    bus->set(ins_stack[value_idx-1], val);
    return true;
}

bool CPU::WB_CR(int value_idx, uint8_t val, uint8_t inc)
{
    uint16_t addr = static_cast<uint16_t>(ins_stack[value_idx-1])<<8;
    addr += ins_stack[value_idx-2] + inc;
    if (ins_step < ins_stack.size())
    {
        bus->set(addr, ins_stack[value_idx]);
        return false;
    }
    bus->set(addr, val);
    return true;
}

bool CPU::WB_NC(int value_idx, uint8_t val, uint8_t inc)
{
    uint16_t addr = (ins_stack[value_idx-1]<<8)
        | (uint8_t)(ins_stack[value_idx-2] + inc);
    if (ins_step < ins_stack.size())
    {
        bus->set(addr, ins_stack[value_idx]);
        return false;
    }
    bus->set(addr, val);
    return true;
}

void CPU::clock_cycle()
{
    if (ins_stack.size() == 0)
    {
        ins_stack.push_back(fetch(true));
        ++ins_step;
        last_p = p;
        return;
    }

    int init_val = 0;
    int comp_val = 0;
    bool complete = false;
    switch ((ins_stack[0]))
    {
    case 0x00:
        // BRK impl
        init_val = ADDR_IMP();
        if (!init_val) break;
        comp_val = OP_NOP(init_val);
        complete = WB_BRK();
        break;
    case 0x01:
        // ORA X,ind
        init_val = ADDR_INX();
        if (!init_val) break;
        comp_val = OP_ORA(init_val);
        complete = WB_ACC(comp_val);
        break;
    case 0x05:
        // ORA zpg
        init_val = ADDR_ZP();
        if (!init_val) break;
        comp_val = OP_ORA(init_val);
        complete = WB_ACC(comp_val);
        break;
    case 0x06:
        // ASL zpg
        init_val = ADDR_ZP();
        if (!init_val) break;
        comp_val = OP_ASL(init_val);
        complete = WB_ZP(init_val, comp_val);
        break;
    case 0x08:
        // PHP impl
        init_val = ADDR_IMP();
        if (!init_val) break;
        comp_val = OP_NOP(init_val);
        complete = WB_PHP();
        break;
    case 0x09:
        // ORA #
        init_val = ADDR_IM();
        if (!init_val) break;
        comp_val = OP_ORA(init_val);
        complete = WB_ACC(comp_val);
        break;
    case 0x0A:
        // ASL A
        init_val = ADDR_IMP();
        if (!init_val) break;
        comp_val = OP_ASL(init_val);
        complete = WB_ACC(comp_val);
        break;
    case 0x0D:
        // ORA abs
        init_val = ADDR_AB();
        if (!init_val) break;
        comp_val = OP_ORA(init_val);
        complete = WB_ACC(comp_val);
        break;
    case 0x0E:
        // ASL abs
        init_val = ADDR_AB();
        if (!init_val) break;
        comp_val = OP_ASL(init_val);
        complete = WB_NC(init_val, comp_val);
        break;
    case 0x10:
        // BPL rel
        break;
    case 0x11:
        // ORA ind,y
        init_val = ADDR_INY();
        if (!init_val) break;
        comp_val = OP_ORA(init_val);
        complete = WB_ACC(comp_val);
        break;
    case 0x15:
        // ORA zpg,x
        init_val = ADDR_ZPX();
        if (!init_val) break;
        comp_val = OP_ORA(init_val);
        complete = WB_ACC(comp_val);
        break;
    case 0x16:
        // ASL zpg,x
        init_val = ADDR_ZPX();
        if (!init_val) break;
        comp_val = OP_ASL(init_val);
        complete = WB_ZP(init_val, comp_val);
        break;
    case 0x18:
        // CLC impl
        break;
    case 0x19:
        // ORA abs,Y
        init_val = ADDR_ABY();
        if (!init_val) break;
        comp_val = OP_ORA(init_val);
        complete = WB_ACC(comp_val);
        break;
    case 0x1D:
        // ORA abs,X
        init_val = ADDR_ABX();
        if (!init_val) break;
        comp_val = OP_ORA(init_val);
        complete = WB_ACC(comp_val);
        break;
    case 0x1E:
        // ASL abs,X
        init_val = ADDR_ABX(false);
        if (!init_val) break;
        comp_val = OP_ASL(init_val);
        complete = WB_CR(init_val, comp_val, x);
        break;
    case 0x20:
        // JSR abs
        break;
    case 0x21:
        // AND X,ind
        init_val = ADDR_INX();
        if (!init_val) break;
        comp_val = OP_AND(init_val);
        complete = WB_ACC(comp_val);
        break;
    case 0x24:
        // BIT zpg
        break;
    case 0x25:
        // AND zpg
        init_val = ADDR_ZP();
        if (!init_val) break;
        comp_val = OP_AND(init_val);
        complete = WB_ACC(comp_val);
        break;
    case 0x26:
        // ROL zpg
        init_val = ADDR_ZP();
        if (!init_val) break;
        comp_val = OP_ROL(init_val);
        complete = WB_ZP(init_val, comp_val);
        break;
    case 0x28:
        // PLP impl
        break;
    case 0x29:
        // AND #
        init_val = ADDR_IM();
        if (!init_val) break;
        comp_val = OP_AND(init_val);
        complete = WB_ACC(comp_val);
        break;
    case 0x2A:
        // ROL A
        init_val = ADDR_IMP();
        if (!init_val) break;
        comp_val = OP_ROL(init_val);
        complete = WB_ACC(comp_val);
        break;
    case 0x2C:
        // BIT abs
        break;
    case 0x2D:
        // AND abs
        init_val = ADDR_AB();
        if (!init_val) break;
        comp_val = OP_AND(init_val);
        complete = WB_ACC(comp_val);
        break;
    case 0x2E:
        // ROL abs
        init_val = ADDR_AB();
        if (!init_val) break;
        comp_val = OP_ROL(init_val);
        complete = WB_NC(init_val, comp_val);
        break;
    case 0x30:
        // BMI rel
        break;
    case 0x31:
        // AND ind,Y
        init_val = ADDR_INY();
        if (!init_val) break;
        comp_val = OP_AND(init_val);
        complete = WB_ACC(comp_val);
        break;
    case 0x35:
        // AND zpg,X
        init_val = ADDR_ZPX();
        if (!init_val) break;
        comp_val = OP_AND(init_val);
        complete = WB_ACC(comp_val);
        break;
    case 0x36:
        // ROL zpg,X
        init_val = ADDR_ZPX();
        if (!init_val) break;
        comp_val = OP_ROL(init_val);
        complete = WB_ZP(init_val, comp_val);
        break;
    case 0x38:
        // SEC impl
        break;
    case 0x39:
        // AND abs,Y
        init_val = ADDR_ABY();
        if (!init_val) break;
        comp_val = OP_AND(init_val);
        complete = WB_ACC(comp_val);
        break;
    case 0x3D:
        // AND abs,X
        init_val = ADDR_ABX();
        if (!init_val) break;
        comp_val = OP_AND(init_val);
        complete = WB_ACC(comp_val);
        break;
    case 0x3E:
        // ROL abs,X
        init_val = ADDR_ABX(false);
        if (!init_val) break;
        comp_val = OP_ROL(init_val);
        complete = WB_CR(init_val, comp_val, x);
        break;
    case 0x40:
        // RTI impl
        break;
    case 0x41:
        // EOR X,ind
        init_val = ADDR_INX();
        if (!init_val) break;
        comp_val = OP_EOR(init_val);
        complete = WB_ACC(comp_val);
        break;
    case 0x45:
        // EOR zpg
        init_val = ADDR_ZP();
        if (!init_val) break;
        comp_val = OP_EOR(init_val);
        complete = WB_ACC(comp_val);
        break;
    case 0x46:
        // LSR zpg
        break;
    case 0x48:
        // PHA impl
        init_val = ADDR_IMP();
        if (!init_val) break;
        comp_val = OP_NOP(init_val);
        complete = WB_PHA();
        break;
    case 0x49:
        // EOR #
        init_val = ADDR_IM();
        if (!init_val) break;
        comp_val = OP_EOR(init_val);
        complete = WB_ACC(comp_val);
        break;
    case 0x4A:
        // LSR A
        break;
    case 0x4C:
        // JMP abs
        break;
    case 0x4D:
        // EOR abs
        init_val = ADDR_AB();
        if (!init_val) break;
        comp_val = OP_EOR(init_val);
        complete = WB_ACC(comp_val);
        break;
    case 0x4E:
        // LSR abs
        break;
    case 0x50:
        // BVC rel
        break;
    case 0x51:
        // EOR ind,Y
        init_val = ADDR_INY();
        if (!init_val) break;
        comp_val = OP_EOR(init_val);
        complete = WB_ACC(comp_val);
        break;
    case 0x55:
        // EOR zpg,X
        init_val = ADDR_ZPX();
        if (!init_val) break;
        comp_val = OP_EOR(init_val);
        complete = WB_ACC(comp_val);
        break;
    case 0x56:
        // LSR zpg,X
        break;
    case 0x58:
        // CLI impl
        break;
    case 0x59:
        // EOR abs,Y
        init_val = ADDR_ABY();
        if (!init_val) break;
        comp_val = OP_EOR(init_val);
        complete = WB_ACC(comp_val);
        break;
    case 0x5D:
        // EOR abs,X
        init_val = ADDR_ABX();
        if (!init_val) break;
        comp_val = OP_EOR(init_val);
        complete = WB_ACC(comp_val);
        break;
    case 0x5E:
        // LSR abs,X
        break;
    case 0x60:
        // RTS impl
        break;
    case 0x61:
        // ADC X,ind
        break;
    case 0x65:
        // ADC zpg
        break;
    case 0x66:
        // ROR zpg
        init_val = ADDR_ZP();
        if (!init_val) break;
        comp_val = OP_ROR(init_val);
        complete = WB_ZP(init_val, comp_val);
        break;
    case 0x68:
        // PLA impl
        break;
    case 0x69:
        // ADC #
        break;
    case 0x6A:
        // ROR A
        init_val = ADDR_IMP();
        if (!init_val) break;
        comp_val = OP_ROR(init_val);
        complete = WB_ACC(comp_val);
        break;
    case 0x6C:
        // JMP ind
        break;
    case 0x6D:
        // ADC abs
        break;
    case 0x6E:
        // ROR abs
        init_val = ADDR_AB();
        if (!init_val) break;
        comp_val = OP_ROR(init_val);
        complete = WB_NC(init_val, comp_val);
        break;
    case 0x70:
        // BVS rel
        break;
    case 0x71:
        // ADC ind,Y
        break;
    case 0x75:
        // ADC zpg,X
        break;
    case 0x76:
        // ROR zpg,X
        init_val = ADDR_ZPX();
        if (!init_val) break;
        comp_val = OP_ROR(init_val);
        complete = WB_ZP(init_val, comp_val);
        break;
    case 0x78:
        // ADC abs,Y
        break;
    case 0x79:
        // ADC abs,Y
        break;
    case 0x7D:
        // ADC abs,X
        break;
    case 0x7E:
        // ROR abs,X
        init_val = ADDR_ABX(false);
        if (!init_val) break;
        comp_val = OP_ROR(init_val);
        complete = WB_CR(init_val, comp_val, x);
        break;
    case 0x81:
        // STA X,ind
        break;
    case 0x84:
        // STY zpg
        break;
    case 0x85:
        // STA zpg
        break;
    case 0x86:
        // STX zpg
        break;
    case 0x88:
        // DEY impl
        break;
    case 0x8A:
        // TXA impl
        break;
    case 0x8C:
        // STY abs
        break;
    case 0x8D:
        // STA abs
        break;
    case 0x8E:
        // STX abs
        break;
    case 0x90:
        // BCC rel
        break;
    case 0x91:
        // STA ind,Y
        break;
    case 0x94:
        // STY zpg,X
        break;
    case 0x95:
        // STA zpg,X
        break;
    case 0x96:
        // STX zpg,y
        break;
    case 0x98:
        // TYA impl
        break;
    case 0x99:
        // STA abs,Y
        break;
    case 0x9A:
        // TXS impl
        break;
    case 0x9D:
        // STA abs,X
        break;
    case 0xA0:
        // LDY #
        init_val = ADDR_IM();
        if (!init_val) break;
        comp_val = OP_NOP(init_val, true);
        complete = WB_Y(comp_val);
        break;
    case 0xA1:
        // LDA X,ind
        init_val = ADDR_INX();
        if (!init_val) break;
        comp_val = OP_NOP(init_val, true);
        complete = WB_ACC(comp_val);
        break;
    case 0xA2:
        // LDX #
        init_val = ADDR_IM();
        if (!init_val) break;
        comp_val = OP_NOP(init_val, true);
        complete = WB_X(comp_val);
        break;
    case 0xA4:
        // LDY zpg
        init_val = ADDR_ZP();
        if (!init_val) break;
        comp_val = OP_NOP(init_val, true);
        complete = WB_Y(comp_val);
        break;
    case 0xA5:
        // LDA zpg
        init_val = ADDR_ZP();
        if (!init_val) break;
        comp_val = OP_NOP(init_val, true);
        complete = WB_ACC(comp_val);
        break;
    case 0xA6:
        // LDX zpg
        init_val = ADDR_ZP();
        if (!init_val) break;
        comp_val = OP_NOP(init_val, true);
        complete = WB_X(comp_val);
        break;
    case 0xA8:
        // TAY impl
        break;
    case 0xA9:
        // LDA #
        init_val = ADDR_IM();
        if (!init_val) break;
        comp_val = OP_NOP(init_val, true);
        complete = WB_ACC(comp_val);
        break;
    case 0xAA:
        // TAX impl
        break;
    case 0xAC:
        // LDY abs
        init_val = ADDR_AB();
        if (!init_val) break;
        comp_val = OP_NOP(init_val, true);
        complete = WB_Y(comp_val);
        break;
    case 0xAD:
        // LDA abs
        init_val = ADDR_AB();
        if (!init_val) break;
        comp_val = OP_NOP(init_val, true);
        complete = WB_ACC(comp_val);
        break;
    case 0xAE:
        // LDX abs
        init_val = ADDR_AB();
        if (!init_val) break;
        comp_val = OP_NOP(init_val, true);
        complete = WB_X(comp_val);
        break;
    case 0xB0:
        // BCS rel
        break;
    case 0xB1:
        // LDA ind,Y
        init_val = ADDR_INY();
        if (!init_val) break;
        comp_val = OP_NOP(init_val, true);
        complete = WB_ACC(comp_val);
        break;
    case 0xB4:
        // LDY zpg,X
        init_val = ADDR_ZPX();
        if (!init_val) break;
        comp_val = OP_NOP(init_val, true);
        complete = WB_Y(comp_val);
        break;
    case 0xB5:
        // LDA zpg,X
        init_val = ADDR_ZPX();
        if (!init_val) break;
        comp_val = OP_NOP(init_val, true);
        complete = WB_ACC(comp_val);
        break;
    case 0xB6:
        // LDX zpg,Y
        break;
    case 0xB8:
        // CLV impl
        break;
    case 0xB9:
        // LDA abs,Y
        init_val = ADDR_ABY();
        if (!init_val) break;
        comp_val = OP_NOP(init_val, true);
        complete = WB_ACC(comp_val);
        break;
    case 0xBA:
        // TSX impl
        break;
    case 0xBC:
        // LDY abs,X
        init_val = ADDR_ABX();
        if (!init_val) break;
        comp_val = OP_NOP(init_val, true);
        complete = WB_Y(comp_val);
        break;
    case 0xBD:
        // LDA abs,X
        init_val = ADDR_ABX();
        if (!init_val) break;
        comp_val = OP_NOP(init_val, true);
        complete = WB_ACC(comp_val);
        break;
    case 0xBE:
        // LDX abs,Y
        init_val = ADDR_ABY();
        if (!init_val) break;
        comp_val = OP_NOP(init_val, true);
        complete = WB_X(comp_val);
        break;
    case 0xC0:
        // CPY #
        break;
    case 0xC1:
        // CMP X,ind
        break;
    case 0xC4:
        // CPY zpg
        break;
    case 0xC5:
        // CMP zpg
        break;
    case 0xC6:
        // DEC zpg
        init_val = ADDR_ZP();
        if (!init_val) break;
        comp_val = OP_DEC(init_val);
        complete = WB_ZP(init_val, comp_val);
        break;
    case 0xC8:
        // INY impl
        break;
    case 0xC9:
        // CMP #
        break;
    case 0xCA:
        // DEX impl
        break;
    case 0xCC:
        // CPY abs
        break;
    case 0xCD:
        // CMP abs
        break;
    case 0xCE:
        // DEC abs
        init_val = ADDR_AB();
        if (!init_val) break;
        comp_val = OP_DEC(init_val);
        complete = WB_NC(init_val, comp_val);
        break;
    case 0xD0:
        // BNE rel
        break;
    case 0xD1:
        // CMP ind,Y
        break;
    case 0xD5:
        // CMP zpg,X
        break;
    case 0xD6:
        // DEC zpg,X
        init_val = ADDR_ZPX();
        if (!init_val) break;
        comp_val = OP_DEC(init_val);
        complete = WB_ZP(init_val, comp_val);
        break;
    case 0xD8:
        // CLD impl
        break;
    case 0xD9:
        // CMP abs,Y
        break;
    case 0xDD:
        // CMP abs,X
        break;
    case 0xDE:
        // DEC abs,X
        init_val = ADDR_ABX(false);
        if (!init_val) break;
        comp_val = OP_DEC(init_val);
        complete = WB_CR(init_val, comp_val, x);
        break;
    case 0xE0:
        // CPX #
        break;
    case 0xE1:
        // SBC X,ind
        break;
    case 0xE4:
        // CPX zpg
        break;
    case 0xE5:
        // SBC zpg
        break;
    case 0xE6:
        // INC zpg
        init_val = ADDR_ZP();
        if (!init_val) break;
        comp_val = OP_INC(init_val);
        complete = WB_ZP(init_val, comp_val);
        break;
    case 0xE8:
        // INX impl
        break;
    case 0xE9:
        // SBC #
        break;
    case 0xEA:
        // NOP impl
        break;
    case 0xEC:
        // CPX abs
        break;
    case 0xED:
        // SBC abs
        break;
    case 0xEE:
        // INC abs
        init_val = ADDR_AB();
        if (!init_val) break;
        comp_val = OP_INC(init_val);
        complete = WB_NC(init_val, comp_val);
        break;
    case 0xF0:
        // BEQ rel
        break;
    case 0xF1:
        // SBC ind,Y
        break;
    case 0xF5:
        // SBC zpg,X
        break;
    case 0xF6:
        // INC zpg,X
        init_val = ADDR_ZPX();
        if (!init_val) break;
        comp_val = OP_INC(init_val);
        complete = WB_ZP(init_val, comp_val);
        break;
    case 0xF8:
        // SED impl
        break;
    case 0xF9:
        // SBC abs,Y
        break;
    case 0xFD:
        // SBC abs,X
        break;
    case 0xFE:
        // INC abs,X
        init_val = ADDR_ABX(false);
        if (!init_val) break;
        comp_val = OP_INC(init_val);
        complete = WB_CR(init_val, comp_val, x);
        break;
    default:
        init_val = true;
    }

    if (!complete)
    {
        ++ins_step;
        return;
    }

    ins_step = 0;
    ins_stack.clear();
}

int CPU::ADDR_IMP()
{
    switch (ins_step)
    {
    case 1:
        fetch(false);
        ins_stack.push_back(a);
    default:
        return 1;
    }
}

int CPU::ADDR_IM()
{
    switch (ins_step)
    {
    case 1:
        ins_stack.push_back(fetch(true));
    default:
        return 1;
    }
}

int CPU::ADDR_ZP()
{
    switch (ins_step)
    {
    case 1:
        ins_stack.push_back(fetch(true));
        break;
    case 2:
        ins_stack.push_back(bus->get(ins_stack[1]));
    default:
        return 2;
    }
    return 0;
}

int CPU::ADDR_ZPX()
{
    switch (ins_step)
    {
    case 1:
        ins_stack.push_back(fetch(true));
        break;
    case 2:
        bus->get(ins_stack[1]);
        ins_stack.push_back(ins_stack[1] + x);
        break;
    case 3:
        ins_stack.push_back(bus->get(ins_stack[2]));
    default:
        return 3;
    }
    return 0;
}

int CPU::ADDR_AB()
{
    switch (ins_step)
    {
    case 1:
    case 2:
        ins_stack.push_back(fetch(true));
        break;
    case 3:{
        uint16_t addr = (uint16_t)ins_stack[2]<<8 | ins_stack[1];
        ins_stack.push_back(bus->get(addr));
        }
    default:
        return 3;
    }
    return 0;
}

int CPU::ADDR_ABX(bool optimise)
{
    switch (ins_step)
    {
    case 1:
    case 2:
        ins_stack.push_back(fetch(true));
        break;
    case 3:{
        uint16_t addr = (static_cast<uint16_t>(ins_stack[2])<<8) | (uint8_t)(ins_stack[1]+x);
        ins_stack.push_back(bus->get(addr));
        if ((uint8_t)(ins_stack[1]+x) >= x && optimise) return 3;
        }break;
    case 4:
        if ((uint8_t)(ins_stack[1]+x) < x || !optimise)
        {
            uint16_t true_addr = (static_cast<uint16_t>(ins_stack[2])<<8) + ins_stack[1] + x;
            ins_stack.insert(ins_stack.begin()+3, bus->get(true_addr));
        }
    default:
        return 3;
    }
    return 0;
}

int CPU::ADDR_ABY()
{
    switch (ins_step)
    {
    case 1:
    case 2:
        ins_stack.push_back(fetch(true));
        break;
    case 3:{
        uint16_t addr = (static_cast<uint16_t>(ins_stack[2])<<8) | (uint8_t)(ins_stack[1]+y);
        ins_stack.push_back(bus->get(addr));
        if ((uint8_t)(ins_stack[1]+y) >= y) return 3;
        }break;
    case 4:
        if ((uint8_t)(ins_stack[1]+y) < y)
        {
            uint16_t true_addr = (static_cast<uint16_t>(ins_stack[2])<<8) + ins_stack[1] + y;
            ins_stack.push_back(bus->get(true_addr));
        }
    default:
        return 3 + ((uint8_t)(ins_stack[1]+y) < y ? 1 : 0);
    }
    return 0;
}

int CPU::ADDR_INX()
{
    switch (ins_step)
    {
    case 1:
        ins_stack.push_back(fetch(true));
        break;
    case 2:
        bus->get(ins_stack[1]);
        ins_stack.push_back(ins_stack[1] + x);
        break;
    case 3:
    case 4:
        ins_stack.push_back(bus->get(ins_stack[2]++));
        break;
    case 5:{
        uint16_t addr = (static_cast<uint16_t>(ins_stack[4])<<8) | ins_stack[3];
        ins_stack.push_back(bus->get(addr));
        }
    default:
        return 5;
    }
    return 0;
}

int CPU::ADDR_INY()
{
    switch (ins_step)
    {
    case 1:
        ins_stack.push_back(fetch(true));
        break;
    case 2:
    case 3:
        ins_stack.push_back(bus->get(ins_stack[1]++));
        break;
    case 4:{
        uint16_t addr = (static_cast<uint16_t>(ins_stack[3])<<8) | (uint8_t)(ins_stack[2]+y);
        ins_stack.push_back(bus->get(addr));
        if ((uint8_t)(ins_stack[2]+y) >= y) return 4;
        }break;
    case 5:
        if ((uint8_t)(ins_stack[2]+y) < y)
        {
            uint16_t true_addr = (static_cast<uint16_t>(ins_stack[3])<<8) + ins_stack[2] + y;
            ins_stack.push_back(bus->get(true_addr));
        }
    default:
        return 4 + ((uint8_t)(ins_stack[2]+y) < y ? 1 : 0);
    }
    return 0;
}

uint8_t CPU::OP_ORA(int value_idx)
{
    uint8_t result = a | ins_stack[value_idx];

    p = p & ~(FLG_ZRO | FLG_NEG);
    p |= (result == 0) ? FLG_ZRO : 0;
    p |= (result & (1 << 7)) ? FLG_NEG : 0;

    return result;
}

uint8_t CPU::OP_AND(int value_idx)
{
    uint8_t result = a & ins_stack[value_idx];

    p = p & ~(FLG_ZRO | FLG_NEG);
    p |= (result == 0) ? FLG_ZRO : 0;
    p |= (result & (1 << 7)) ? FLG_NEG : 0;

    return result;
}

uint8_t CPU::OP_EOR(int value_idx)
{
    uint8_t result = a ^ ins_stack[value_idx];

    p = p & ~(FLG_ZRO | FLG_NEG);
    p |= (result == 0) ? FLG_ZRO : 0;
    p |= (result & (1 << 7)) ? FLG_NEG : 0;

    return result;
}

uint8_t CPU::OP_ASL(int value_idx)
{
    p = p & ~(FLG_ZRO | FLG_NEG | FLG_CRY);
    p |= (ins_stack[value_idx] & 0x80) ? FLG_CRY : 0;

    uint8_t result = ins_stack[value_idx] << 1;

    p |= (result == 0) ? FLG_ZRO : 0;
    p |= (result & (1 << 7)) ? FLG_NEG : 0;

    return result;
}

uint8_t CPU::OP_ROL(int value_idx)
{

    uint8_t result = ins_stack[value_idx] << 1;
    result |= (last_p & FLG_CRY) ? 1 : 0;

    p = p & ~(FLG_ZRO | FLG_NEG | FLG_CRY);
    p |= (ins_stack[value_idx] & 0x80) ? FLG_CRY : 0;
    p |= (result == 0) ? FLG_ZRO : 0;
    p |= (result & (1 << 7)) ? FLG_NEG : 0;

    return result;
}

uint8_t CPU::OP_ROR(int value_idx)
{

    uint8_t result = ins_stack[value_idx] >> 1;
    result |= (last_p & FLG_CRY) ? (0x80) : 0;

    p = p & ~(FLG_ZRO | FLG_NEG | FLG_CRY);
    p |= (ins_stack[value_idx] & 1) ? FLG_CRY : 0;
    p |= (result == 0) ? FLG_ZRO : 0;
    p |= (result & (1 << 7)) ? FLG_NEG : 0;

    return result;
}

uint8_t CPU::OP_DEC(int value_idx)
{
    uint8_t result = ins_stack[value_idx] - 1;

    p = p & ~(FLG_ZRO | FLG_NEG);
    p |= (result == 0) ? FLG_ZRO : 0;
    p |= (result & (1 << 7)) ? FLG_NEG : 0;

    return result;
}

uint8_t CPU::OP_INC(int value_idx)
{
    uint8_t result = ins_stack[value_idx] + 1;

    p = p & ~(FLG_ZRO | FLG_NEG);
    p |= (result == 0) ? FLG_ZRO : 0;
    p |= (result & (1 << 7)) ? FLG_NEG : 0;

    return result;
}

uint8_t CPU::OP_NOP(int value_idx, bool flag)
{
    if (flag)
    {
        p = p & ~(FLG_ZRO | FLG_NEG);
        p |= (ins_stack[value_idx] == 0) ? FLG_ZRO : 0;
        p |= (ins_stack[value_idx] & (1 << 7)) ? FLG_NEG : 0;
    }
    return ins_stack[value_idx];
}

bool CPU::WB_BRK()
{
    switch(ins_step)
    {
    case 1:
        pc++;
        break;
    case 2:
        bus->set(0x0100+s--, (pc>>8)&0xFF);
        break;
    case 3:
        bus->set(0x0100+s--, pc&0xFF);
        break;
    case 4:
        bus->set(0x0100+s--, p|FLG_BRK);
        p |= FLG_INT;
        break;
    case 5:
        pc = bus->get(0xFFFE);
        break;
    case 6:
        pc |= bus->get(0xFFFF)<<8;
    default:
        return true;
    }
    return false;
}

bool CPU::WB_PHP()
{
    switch(ins_step)
    {
    case 1:
        break;
    case 2:
        bus->set(0x0100+s--, p|FLG_BRK);
    default:
        return true;
    }
    return false;
}

bool CPU::WB_PHA()
{
    switch(ins_step)
    {
    case 1:
        break;
    case 2:
        bus->set(0x0100+s--, a);
    default:
        return true;
    }
    return false;
}