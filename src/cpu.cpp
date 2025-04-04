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
    pc{}, a{}, x{}, y{}, s{}, p{}, bus{nullptr}, ins_step{0}, last_p{}
{}

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

bool CPU::WB_MEM(uint8_t comp_val, bool delay)
{
    if (ins_step < ins_stack.size())
    {
        return false;
    }
    if (ins_step <= ins_stack.size() && delay)
    {
        bus->set(addr, val);
        return false;
    }
    bus->set(addr, comp_val);
    return true;
}

void CPU::clock_cycle()
{
    if (ins_step == 0)
    {
        opcode = fetch(true);
        ++ins_step;
        last_p = p;
        return;
    }

    int comp_val = 0;
    bool complete = true;
    switch (opcode)
    {
    case 0x00:
        // BRK impl
        if (!(complete = ADDR_IMP())) break;
        complete = WB_BRK();
        break;
    case 0x01:
        // ORA X,ind
        if (!(complete = ADDR_INX())) break;
        comp_val = OP_ORA();
        a = comp_val;
        break;
    case 0x05:
        // ORA zpg
        if (!(complete = ADDR_ZP())) break;
        comp_val = OP_ORA();
        a = comp_val;
        break;
    case 0x06:
        // ASL zpg
        if (!(complete = ADDR_ZP())) break;
        comp_val = OP_ASL();
        complete = WB_MEM(comp_val);
        break;
    case 0x08:
        // PHP impl
        if (!(complete = ADDR_IMP())) break;
        complete = WB_PHP();
        break;
    case 0x09:
        // ORA #
        if (!(complete = ADDR_IM())) break;
        comp_val = OP_ORA();
        a = comp_val;
        break;
    case 0x0A:
        // ASL A
        if (!(complete = ADDR_IMP())) break;
        comp_val = OP_ASL();
        a = comp_val;
        break;
    case 0x0D:
        // ORA abs
        if (!(complete = ADDR_AB())) break;
        comp_val = OP_ORA();
        a = comp_val;
        break;
    case 0x0E:
        // ASL abs
        if (!(complete = ADDR_AB())) break;
        comp_val = OP_ASL();
        complete = WB_MEM(comp_val);
        break;
    case 0x10:
        // BPL rel
        if (!(complete = ADDR_REL())) break;
        if (complete = FLG_NEG & p) break;
        complete = BRANCH();
        break;
    case 0x11:
        // ORA ind,y
        if (!(complete = ADDR_INY())) break;
        comp_val = OP_ORA();
        a = comp_val;
        break;
    case 0x15:
        // ORA zpg,x
        if (!(complete = ADDR_ZPX())) break;
        comp_val = OP_ORA();
        a = comp_val;
        break;
    case 0x16:
        // ASL zpg,x
        if (!(complete = ADDR_ZPX())) break;
        comp_val = OP_ASL();
        complete = WB_MEM(comp_val);
        break;
    case 0x18:
        // CLC impl
        if (!(complete = ADDR_IMP())) break;
        complete = clear_flag(FLG_CRY);
        break;
    case 0x19:
        // ORA abs,Y
        if (!(complete = ADDR_ABY())) break;
        comp_val = OP_ORA();
        a = comp_val;
        break;
    case 0x1D:
        // ORA abs,X
        if (!(complete = ADDR_ABX())) break;
        comp_val = OP_ORA();
        a = comp_val;
        break;
    case 0x1E:
        // ASL abs,X
        if (!(complete = ADDR_ABX(false))) break;
        comp_val = OP_ASL();
        complete = WB_MEM(comp_val);
        break;
    case 0x20:
        // JSR abs
        if (!(complete = ADDR_ABP_R())) break;
        complete = WB_JSR();
        break;
    case 0x21:
        // AND X,ind
        if (!(complete = ADDR_INX())) break;
        comp_val = OP_AND();
        a = comp_val;
        break;
    case 0x24:
        // BIT zpg
        if (!(complete = ADDR_ZP())) break;
        comp_val = OP_TST();
        break;
    case 0x25:
        // AND zpg
        if (!(complete = ADDR_ZP())) break;
        comp_val = OP_AND();
        a = comp_val;
        break;
    case 0x26:
        // ROL zpg
        if (!(complete = ADDR_ZP())) break;
        comp_val = OP_ROL();
        complete = WB_MEM(comp_val);
        break;
    case 0x28:
        // PLP impl
        if (!(complete = ADDR_IMP())) break;
        complete = WB_PLP();
        break;
    case 0x29:
        // AND #
        if (!(complete = ADDR_IM())) break;
        comp_val = OP_AND();
        a = comp_val;
        break;
    case 0x2A:
        // ROL A
        if (!(complete = ADDR_IMP())) break;
        comp_val = OP_ROL();
        a = comp_val;
        break;
    case 0x2C:
        // BIT abs
        if (!(complete = ADDR_AB())) break;
        comp_val - OP_TST();
        break;
    case 0x2D:
        // AND abs
        if (!(complete = ADDR_AB())) break;
        comp_val = OP_AND();
        a = comp_val;
        break;
    case 0x2E:
        // ROL abs
        if (!(complete = ADDR_AB())) break;
        comp_val = OP_ROL();
        complete = WB_MEM(comp_val);
        break;
    case 0x30:
        // BMI rel
        if (!(complete = ADDR_REL())) break;
        if (complete = !(FLG_NEG & p)) break;
        complete = BRANCH();
        break;
    case 0x31:
        // AND ind,Y
        if (!(complete = ADDR_INY())) break;
        comp_val = OP_AND();
        a = comp_val;
        break;
    case 0x35:
        // AND zpg,X
        if (!(complete = ADDR_ZPX())) break;
        comp_val = OP_AND();
        a = comp_val;
        break;
    case 0x36:
        // ROL zpg,X
        if (!(complete = ADDR_ZPX())) break;
        comp_val = OP_ROL();
        complete = WB_MEM(comp_val);
        break;
    case 0x38:
        // SEC impl
        if (!(complete = ADDR_IMP())) break;
        complete = set_flag(FLG_CRY);
        break;
    case 0x39:
        // AND abs,Y
        if (!(complete = ADDR_ABY())) break;
        comp_val = OP_AND();
        a = comp_val;
        break;
    case 0x3D:
        // AND abs,X
        if (!(complete = ADDR_ABX())) break;
        comp_val = OP_AND();
        a = comp_val;
        break;
    case 0x3E:
        // ROL abs,X
        if (!(complete = ADDR_ABX(false))) break;
        comp_val = OP_ROL();
        complete = WB_MEM(comp_val);
        break;
    case 0x40:
        // RTI impl
        if (!(complete = ADDR_IMP())) break;
        complete = WB_RTI();
        break;
    case 0x41:
        // EOR X,ind
        if (!(complete = ADDR_INX())) break;
        comp_val = OP_EOR();
        a = comp_val;
        break;
    case 0x45:
        // EOR zpg
        if (!(complete = ADDR_ZP())) break;
        comp_val = OP_EOR();
        a = comp_val;
        break;
    case 0x46:
        // LSR zpg
        if (!(complete = ADDR_ZP())) break;
        comp_val = OP_LSR();
        complete = WB_MEM(comp_val);
        break;
    case 0x48:
        // PHA impl
        if (!(complete = ADDR_IMP())) break;
        complete = WB_PHA();
        break;
    case 0x49:
        // EOR #
        if (!(complete = ADDR_IM())) break;
        comp_val = OP_EOR();
        a = comp_val;
        break;
    case 0x4A:
        // LSR A
        if (!(complete = ADDR_IMP())) break;
        comp_val = OP_LSR();
        a = comp_val;
        break;
    case 0x4C:
        // JMP abs
        if (!(complete = ADDR_AB_R())) break;
        pc = addr;
        break;
    case 0x4D:
        // EOR abs
        if (!(complete = ADDR_AB())) break;
        comp_val = OP_EOR();
        a = comp_val;
        break;
    case 0x4E:
        // LSR abs
        if (!(complete = ADDR_AB())) break;
        comp_val = OP_LSR();
        complete = WB_MEM(comp_val);
        break;
    case 0x50:
        // BVC rel
        if (!(complete = ADDR_REL())) break;
        if (complete = FLG_OVR & p) break;
        complete = BRANCH();
        break;
    case 0x51:
        // EOR ind,Y
        if (!(complete = ADDR_INY())) break;
        comp_val = OP_EOR();
        a = comp_val;
        break;
    case 0x55:
        // EOR zpg,X
        if (!(complete = ADDR_ZPX())) break;
        comp_val = OP_EOR();
        a = comp_val;
        break;
    case 0x56:
        // LSR zpg,X
        if (!(complete = ADDR_ZPX())) break;
        comp_val = OP_LSR();
        complete = WB_MEM(comp_val);
        break;
    case 0x58:
        // CLI impl
        if (!(complete = ADDR_IMP())) break;
        complete = clear_flag(FLG_INT);
        break;
    case 0x59:
        // EOR abs,Y
        if (!(complete = ADDR_ABY())) break;
        comp_val = OP_EOR();
        a = comp_val;
        break;
    case 0x5D:
        // EOR abs,X
        if (!(complete = ADDR_ABX())) break;
        comp_val = OP_EOR();
        a = comp_val;
        break;
    case 0x5E:
        // LSR abs,X
        if (!(complete = ADDR_ABX(false))) break;
        comp_val = OP_LSR();
        complete = WB_MEM(comp_val);
        break;
    case 0x60:
        // RTS impl
        if (!(complete = ADDR_IMP())) break;
        complete = WB_RTS();
        break;
    case 0x61:
        // ADC X,ind
        if (!(complete = ADDR_INX())) break;
        comp_val = OP_ADC();
        a = comp_val;
        break;
    case 0x65:
        // ADC zpg
        if (!(complete = ADDR_ZP())) break;
        comp_val = OP_ADC();
        a = comp_val;
        break;
    case 0x66:
        // ROR zpg
        if (!(complete = ADDR_ZP())) break;
        comp_val = OP_ROR();
        complete = WB_MEM(comp_val);
        break;
    case 0x68:
        // PLA impl
        if (!(complete = ADDR_IMP())) break;
        complete = WB_PLA();
        break;
    case 0x69:
        // ADC #
        if (!(complete = ADDR_IM())) break;
        comp_val = OP_ADC();
        a = comp_val;
        break;
    case 0x6A:
        // ROR A
        if (!(complete = ADDR_IMP())) break;
        comp_val = OP_ROR();
        a = comp_val;
        break;
    case 0x6C:
        // JMP ind
        if (!(complete = ADDR_IN_R())) break;
        pc = addr;
        break;
    case 0x6D:
        // ADC abs
        if (!(complete = ADDR_AB())) break;
        comp_val = OP_ADC();
        a = comp_val;
        break;
    case 0x6E:
        // ROR abs
        if (!(complete = ADDR_AB())) break;
        comp_val = OP_ROR();
        complete = WB_MEM(comp_val);
        break;
    case 0x70:
        // BVS rel
        if (!(complete = ADDR_REL())) break;
        if (complete = !(FLG_OVR & p)) break;
        complete = BRANCH();
        break;
    case 0x71:
        // ADC ind,Y
        if (!(complete = ADDR_INY())) break;
        comp_val = OP_ADC();
        a = comp_val;
        break;
    case 0x75:
        // ADC zpg,X
        if (!(complete = ADDR_ZPX())) break;
        comp_val = OP_ADC();
        a = comp_val;
        break;
    case 0x76:
        // ROR zpg,X
        if (!(complete = ADDR_ZPX())) break;
        comp_val = OP_ROR();
        complete = WB_MEM(comp_val);
        break;
    case 0x78:
        // SEI impl
        if (!(complete = ADDR_IMP())) break;
        complete = set_flag(FLG_INT);
        break;
    case 0x79:
        // ADC abs,Y
        if (!(complete = ADDR_ABY())) break;
        comp_val = OP_ADC();
        a = comp_val;
        break;
    case 0x7D:
        // ADC abs,X
        if (!(complete = ADDR_ABX())) break;
        comp_val = OP_ADC();
        a = comp_val;
        break;
    case 0x7E:
        // ROR abs,X
        if (!(complete = ADDR_ABX(false))) break;
        comp_val = OP_ROR();
        complete = WB_MEM(comp_val);
        break;
    case 0x81:
        // STA X,ind
        if (!(complete = ADDR_INX_R())) break;
        complete = WB_MEM(a, false);
        break;
    case 0x84:
        // STY zpg
        if (!(complete = ADDR_ZP_R())) break;
        complete = WB_MEM(y, false);
        break;
    case 0x85:
        // STA zpg
        if (!(complete = ADDR_ZP_R())) break;
        complete = WB_MEM(a, false);
        break;
    case 0x86:
        // STX zpg
        if (!(complete = ADDR_ZP_R())) break;
        complete = WB_MEM(x, false);
        break;
    case 0x88:
        // DEY impl
        if (!(complete = ADDR_IMP())) break;
        val = y;
        comp_val = OP_DEC();
        y = comp_val;
        break;
    case 0x8A:
        // TXA impl
        if (!(complete = ADDR_IMP())) break;
        OP_FLG(x);
        a = x;
        break;
    case 0x8C:
        // STY abs
        if (!(complete = ADDR_AB_R())) break;
        complete = WB_MEM(y, false);
        break;
    case 0x8D:
        // STA abs
        if (!(complete = ADDR_AB_R())) break;
        complete = WB_MEM(a, false);
        break;
    case 0x8E:
        // STX abs
        if (!(complete = ADDR_AB_R())) break;
        complete = WB_MEM(x, false);
        break;
    case 0x90:
        // BCC rel
        if (!(complete = ADDR_REL())) break;
        if (complete = FLG_CRY & p) break;
        complete = BRANCH();
        break;
    case 0x91:
        // STA ind,Y
        if (!(complete = ADDR_INY_R(false))) break;
        complete = WB_MEM(a, false);
        break;
    case 0x94:
        // STY zpg,X
        if (!(complete = ADDR_ZPX_R())) break;
        complete = WB_MEM(y, false);
        break;
    case 0x95:
        // STA zpg,X
        if (!(complete = ADDR_ZPX_R())) break;
        complete = WB_MEM(a, false);
        break;
    case 0x96:
        // STX zpg,y
        if (!(complete = ADDR_ZPY_R())) break;
        complete = WB_MEM(x, false);
        break;
    case 0x98:
        // TYA impl
        if (!(complete = ADDR_IMP())) break;
        OP_FLG(y);
        a = y;
        break;
    case 0x99:
        // STA abs,Y
        if (!(complete = ADDR_ABY_R(false))) break;
        complete = WB_MEM(a, false);
        break;
    case 0x9A:
        // TXS impl
        if (!(complete = ADDR_IMP())) break;
        s = x;
        break;
    case 0x9D:
        // STA abs,X
        if (!(complete = ADDR_ABX_R(false))) break;
        complete = WB_MEM(a, false);
        break;
    case 0xA0:
        // LDY #
        if (!(complete = ADDR_IM())) break;
        OP_FLG(val);
        y = val;
        break;
    case 0xA1:
        // LDA X,ind
        if (!(complete = ADDR_INX())) break;
        OP_FLG(val);
        a = val;
        break;
    case 0xA2:
        // LDX #
        if (!(complete = ADDR_IM())) break;
        OP_FLG(val);
        x = val;
        break;
    case 0xA4:
        // LDY zpg
        if (!(complete = ADDR_ZP())) break;
        OP_FLG(val);
        y = val;
        break;
    case 0xA5:
        // LDA zpg
        if (!(complete = ADDR_ZP())) break;
        OP_FLG(val);
        a = val;
        break;
    case 0xA6:
        // LDX zpg
        if (!(complete = ADDR_ZP())) break;
        OP_FLG(val);
        x = val;
        break;
    case 0xA8:
        // TAY impl
        if (!(complete = ADDR_IMP())) break;
        OP_FLG(a);
        y = a;
        break;
    case 0xA9:
        // LDA #
        if (!(complete = ADDR_IM())) break;
        OP_FLG(val);
        a = val;
        break;
    case 0xAA:
        // TAX impl
        if (!(complete = ADDR_IMP())) break;
        OP_FLG(a);
        x = a;
        break;
    case 0xAC:
        // LDY abs
        if (!(complete = ADDR_AB())) break;
        OP_FLG(val);
        y = val;
        break;
    case 0xAD:
        // LDA abs
        if (!(complete = ADDR_AB())) break;
        OP_FLG(val);
        a = val;
        break;
    case 0xAE:
        // LDX abs
        if (!(complete = ADDR_AB())) break;
        OP_FLG(val);
        x = val;
        break;
    case 0xB0:
        // BCS rel
        if (!(complete = ADDR_REL())) break;
        if (complete = !(FLG_CRY & p)) break;
        complete = BRANCH();
        break;
    case 0xB1:
        // LDA ind,Y
        if (!(complete = ADDR_INY())) break;
        OP_FLG(val);
        a = val;
        break;
    case 0xB4:
        // LDY zpg,X
        if (!(complete = ADDR_ZPX())) break;
        OP_FLG(val);
        y = val;
        break;
    case 0xB5:
        // LDA zpg,X
        if (!(complete = ADDR_ZPX())) break;
        OP_FLG(val);
        a = val;
        break;
    case 0xB6:
        // LDX zpg,Y
        if (!(complete = ADDR_ZPY())) break;
        OP_FLG(val);
        x = val;
        break;
    case 0xB8:
        // CLV impl
        if (!(complete = ADDR_IMP())) break;
        complete = clear_flag(FLG_OVR);
        break;
    case 0xB9:
        // LDA abs,Y
        if (!(complete = ADDR_ABY())) break;
        OP_FLG(val);
        a = val;
        break;
    case 0xBA:
        // TSX impl
        if (!(complete = ADDR_IMP())) break;
        OP_FLG(s);
        x = s;
        break;
    case 0xBC:
        // LDY abs,X
        if (!(complete = ADDR_ABX())) break;
        OP_FLG(val);
        y = val;
        break;
    case 0xBD:
        // LDA abs,X
        if (!(complete = ADDR_ABX())) break;
        OP_FLG(val);
        a = val;
        break;
    case 0xBE:
        // LDX abs,Y
        if (!(complete = ADDR_ABY())) break;
        OP_FLG(val);
        x = val;
        break;
    case 0xC0:
        // CPY #
        if (!(complete = ADDR_IM())) break;
        comp_val = OP_CMP(y);
        break;
    case 0xC1:
        // CMP X,ind
        if (!(complete = ADDR_INX())) break;
        comp_val = OP_CMP(a);
        break;
    case 0xC4:
        // CPY zpg
        if (!(complete = ADDR_ZP())) break;
        comp_val = OP_CMP(y);
        break;
    case 0xC5:
        // CMP zpg
        if (!(complete = ADDR_ZP())) break;
        comp_val = OP_CMP(a);
        break;
    case 0xC6:
        // DEC zpg
        if (!(complete = ADDR_ZP())) break;
        comp_val = OP_DEC();
        complete = WB_MEM(comp_val);
        break;
    case 0xC8:
        // INY impl
        if (!(complete = ADDR_IMP())) break;
        val = y;
        comp_val = OP_INC();
        y = comp_val;
        break;
    case 0xC9:
        // CMP #
        if (!(complete = ADDR_IM())) break;
        comp_val = OP_CMP(a);
        break;
    case 0xCA:
        // DEX impl
        if (!(complete = ADDR_IMP())) break;
        val = x;
        comp_val = OP_DEC();
        x = comp_val;
        break;
    case 0xCC:
        // CPY abs
        if (!(complete = ADDR_AB())) break;
        comp_val = OP_CMP(y);
        break;
    case 0xCD:
        // CMP abs
        if (!(complete = ADDR_AB())) break;
        comp_val = OP_CMP(a);
        break;
    case 0xCE:
        // DEC abs
        if (!(complete = ADDR_AB())) break;
        comp_val = OP_DEC();
        complete = WB_MEM(comp_val);
        break;
    case 0xD0:
        // BNE rel
        if (!(complete = ADDR_REL())) break;
        if (complete = FLG_ZRO & p) break;
        complete = BRANCH();
        break;
    case 0xD1:
        // CMP ind,Y
        if (!(complete = ADDR_INY())) break;
        comp_val = OP_CMP(a);
        break;
    case 0xD5:
        // CMP zpg,X
        if (!(complete = ADDR_ZPX())) break;
        comp_val = OP_CMP(a);
        break;
    case 0xD6:
        // DEC zpg,X
        if (!(complete = ADDR_ZPX())) break;
        comp_val = OP_DEC();
        complete = WB_MEM(comp_val);
        break;
    case 0xD9:
        // CMP abs,Y
        if (!(complete = ADDR_ABY())) break;
        comp_val = OP_CMP(a);
        break;
    case 0xDD:
        // CMP abs,X
        if (!(complete = ADDR_ABX())) break;
        comp_val = OP_CMP(a);
        break;
    case 0xDE:
        // DEC abs,X
        if (!(complete = ADDR_ABX(false))) break;
        comp_val = OP_DEC();
        complete = WB_MEM(comp_val);
        break;
    case 0xE0:
        // CPX #
        if (!(complete = ADDR_IM())) break;
        comp_val = OP_CMP(x);
        break;
    case 0xE1:
        // SBC X,ind
        if (!(complete = ADDR_INX())) break;
        comp_val = OP_SBC();
        a = comp_val;
        break;
    case 0xE4:
        // CPX zpg
        if (!(complete = ADDR_ZP())) break;
        comp_val = OP_CMP(x);
        break;
    case 0xE5:
        // SBC zpg
        if (!(complete = ADDR_ZP())) break;
        comp_val = OP_SBC();
        a = comp_val;
        break;
    case 0xE6:
        // INC zpg
        if (!(complete = ADDR_ZP())) break;
        comp_val = OP_INC();
        complete = WB_MEM(comp_val);
        break;
    case 0xE8:
        // INX impl
        if (!(complete = ADDR_IMP())) break;
        val = x;
        comp_val = OP_INC();
        x = comp_val;
        break;
    case 0xE9:
        // SBC #
        if (!(complete = ADDR_IM())) break;
        comp_val = OP_SBC();
        a = comp_val;
        break;
    case 0xEA:
        // NOP impl
        if (!(complete = ADDR_IMP())) break;
        break;
    case 0xEC:
        // CPX abs
        if (!(complete = ADDR_AB())) break;
        comp_val = OP_CMP(x);
        break;
    case 0xED:
        // SBC abs
        if (!(complete = ADDR_AB())) break;
        comp_val = OP_SBC();
        a = comp_val;
        break;
    case 0xEE:
        // INC abs
        if (!(complete = ADDR_AB())) break;
        comp_val = OP_INC();
        complete = WB_MEM(comp_val);
        break;
    case 0xF0:
        // BEQ rel
        if (!(complete = ADDR_REL())) break;
        if (complete = !(FLG_ZRO & p)) break;
        complete = BRANCH();
        break;
    case 0xF1:
        // SBC ind,Y
        if (!(complete = ADDR_INY())) break;
        comp_val = OP_SBC();
        a = comp_val;
        break;
    case 0xF5:
        // SBC zpg,X
        if (!(complete = ADDR_ZPX())) break;
        comp_val = OP_SBC();
        a = comp_val;
        break;
    case 0xF6:
        // INC zpg,X
        if (!(complete = ADDR_ZPX())) break;
        comp_val = OP_INC();
        complete = WB_MEM(comp_val);
        break;
    case 0xF9:
        // SBC abs,Y
        if (!(complete = ADDR_ABY())) break;
        comp_val = OP_SBC();
        a = comp_val;
        break;
    case 0xFD:
        // SBC abs,X
        if (!(complete = ADDR_ABX())) break;
        comp_val = OP_SBC();
        a = comp_val;
        break;
    case 0xFE:
        // INC abs,X
        if (!(complete = ADDR_ABX(false))) break;
        comp_val = OP_INC();
        complete = WB_MEM(comp_val);
        break;
    default:
        complete = true;
    }

    if (!complete)
    {
        ++ins_step;
        return;
    }
    ins_step = 0;
}

bool CPU::ADDR_IMP()
{
    switch (ins_step)
    {
    case 1:
        fetch(false);
        val = a;
    default:
        return true;
    }
    return false;
}

bool CPU::ADDR_IM()
{
    switch (ins_step)
    {
    case 1:
        val = fetch(true);
    default:
        return true;
    }
    return false;
}

bool CPU::ADDR_ZP()
{
    switch (ins_step)
    {
    case 1:
        addr = fetch(true);
        break;
    case 2:
        val = bus->get(addr);
    default:
        return true;
    }
    return false;
}

bool CPU::ADDR_ZP_R()
{
    switch (ins_step)
    {
    case 1:
        addr = fetch(true);
    default:
        return true;
    }
    return false;
}

bool CPU::ADDR_ZPX()
{
    switch (ins_step)
    {
    case 1:
        addr = fetch(true);
        break;
    case 2:
        bus->get(addr);
        addr = static_cast<uint8_t>(addr + x);
        break;
    case 3:
        val = bus->get(addr);
    default:
        return true;
    }
    return false;
}

bool CPU::ADDR_ZPY()
{
    switch (ins_step)
    {
    case 1:
        addr = fetch(true);
        break;
    case 2:
        bus->get(addr);
        addr = static_cast<uint8_t>(addr + y);
        break;
    case 3:
        val = bus->get(addr);
    default:
        return true;
    }
    return false;
}

bool CPU::ADDR_ZPX_R()
{
    switch (ins_step)
    {
    case 1:
        addr = fetch(true);
        break;
    case 2:
        bus->get(addr);
        addr = static_cast<uint8_t>(addr + x);
    default:
        return true;
    }
    return false;
}

bool CPU::ADDR_ZPY_R()
{
    switch (ins_step)
    {
    case 1:
        addr = fetch(true);
        break;
    case 2:
        bus->get(addr);
        addr = static_cast<uint8_t>(addr + y);
    default:
        return true;
    }
    return false;
}

bool CPU::ADDR_AB()
{
    switch (ins_step)
    {
    case 1:
    case 2:
        addr >>= 8;
        addr |= fetch(true)<<;
        break;
    case 3:
        val = bus->get(addr);
    default:
        return true;
    }
    return false;
}

bool CPU::ADDR_AB_R()
{
    switch (ins_step)
    {
    case 1:
        addr = fetch(true)<<8;
        break;
    case 2:
        addr >>= 8;
        addr |= fetch(true)<<8;
    default:
        return true;
    }
    return false;
}

bool CPU::ADDR_ABX(bool optimise)
{
    switch (ins_step)
    {
    case 1:
    case 2:
        addr >>= 8;
        addr |= fetch(true)<<8;
        break;
    case 3:
        if ((uint8_t)(addr&0xFF + x) >= x && optimise)
        {
            val = bus->get(addr += x);
            return true;
        }
        else
        {
            bus->get(addr&0xFF00 | static_cast<uint8_t>(addr&0xFF+x));
        }
        break;
    case 4:
        if ((uint8_t)(addr&0xFF + x) < x || !optimise)
        {
            val = bus->get(addr += x);
        }
    default:
        return true;
    }
    return false;
}

bool CPU::ADDR_ABX_R(bool optimise)
{
    switch (ins_step)
    {
    case 1:
        addr = fetch(true)<<8;
        break;
    case 2:
        addr >>= 8;
        addr |= fetch(true)<<8;
        if ((uint8_t)(addr&0xFF + x) >= x && optimise)
        {
            addr += x;
            return true;
        }
        break;
    case 3:
        if ((uint8_t)(addr&0xFF + x) < x || !optimise)
        {
            bus->get(addr&0xFF00 | static_cast<uint8_t>(addr&0xFF+x));
            addr += x;
        }
    default:
        return true;
    }
    return false;
}

bool CPU::ADDR_ABY()
{
    switch (ins_step)
    {
    case 1:
    case 2:
        addr >>= 8;
        addr |= fetch(true)<<8;
        break;
    case 3:
        if ((uint8_t)(addr&0xFF + y) >= y)
        {
            val = bus->get(addr += y);
            return true;
        }
        else
        {
            bus->get(addr&0xFF00 | static_cast<uint8_t>(addr&0xFF+y));
        }
        break;
    case 4:
        if ((uint8_t)(addr&0xFF + y) < y)
        {
            val = bus->get(addr += y);
        }
    default:
        return true;
    }
    return false;
}

bool CPU::ADDR_ABY_R(bool optimise)
{
    switch (ins_step)
    {
    case 1:
        addr = fetch(true);
        break;
    case 2:
        addr <<= 8;
        addr |= fetch(true);
        if ((uint8_t)(addr&0xFF + y) >= y && optimise)
        {
            addr += y;
            return true;
        }
        break;
    case 3:
        if ((uint8_t)(addr&0xFF + y) < y || !optimise)
        {
            bus->get(addr&0xFF00 | static_cast<uint8_t>(addr&0xFF+y));
            addr += y;
        }
    default:
        return true;
    }
    return false;
}

bool CPU::ADDR_INX()
{
    switch (ins_step)
    {
    case 1:
        addr = fetch(true);
        break;
    case 2:
        bus->get(addr);
        addr = static_cast<uint8_t>(addr + x);
        break;
    case 3:
        addr |= bus->get(addr&0xFF)<<8;
        addr = addr&0xFF00 | static_cast<uint8_t>(addr+1);
        break;
    case 4:
        addr = addr&0xFF00 | bus->get(addr&0xFF);
        break;
    case 5:
        val = bus->get(addr);
    default:
        return true;
    }
    return false;
}

bool CPU::ADDR_IN_R()
{
    switch (ins_step)
    {
    case 1:
    case 2:
        ins_stack.push_back(fetch(true));
        break;
    case 3:
        addr = bus->get((ins_stack[2]<<8) | ins_stack[1]++);
        break;
    case 4:
        addr |= (bus->get((ins_stack[2]<<8) | ins_stack[1]))<<8;
    default:
        return true;
    }
    return false;
}

bool CPU::ADDR_INX_R()
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
        break;
    case 4:
        ins_stack.push_back(bus->get((uint8_t)(ins_stack[2]+1)));
        addr = (static_cast<uint16_t>(ins_stack[4])<<8) | ins_stack[3];
    default:
        return true;
    }
    return false;
}

bool CPU::ADDR_INY()
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
    case 4:
        addr = (static_cast<uint16_t>(ins_stack[3])<<8) | (uint8_t)(ins_stack[2]+y);
        ins_stack.push_back(bus->get(addr));
        val = ins_stack[4];
        if ((uint8_t)(ins_stack[2]+y) >= y) return true;
        break;
    case 5:
        if ((uint8_t)(ins_stack[2]+y) < y)
        {
            addr = (static_cast<uint16_t>(ins_stack[3])<<8) + ins_stack[2] + y;
            ins_stack.push_back(bus->get(addr));
            val = ins_stack[5];
        }
    default:
        return true;
    }
    return false;
}

bool CPU::ADDR_INY_R(bool optimise)
{
    switch (ins_step)
    {
    case 1:
        ins_stack.push_back(fetch(true));
        break;
    case 2:
        ins_stack.push_back(bus->get(ins_stack[1]++));
        break;
    case 3:
        ins_stack.push_back(bus->get(ins_stack[1]++));
        addr = (static_cast<uint16_t>(ins_stack[3])<<8) | (uint8_t)(ins_stack[2]+y);
        if ((uint8_t)(ins_stack[2]+y) >= y && optimise) return true;
        break;
    case 4:
        if ((uint8_t)(ins_stack[2]+y) < y || !optimise)
        {
            ins_stack.push_back(bus->get(addr));
            addr = (static_cast<uint16_t>(ins_stack[3])<<8) + ins_stack[2] + y;
        }
    default:
        return true;
    }
    return false;
}

bool CPU::ADDR_REL()
{
    switch(ins_step)
    {
    case 1:
        ins_stack.push_back(fetch(true));
        addr = ins_stack[1];
    default:
        return true;
    }
    return false;
}

bool CPU::ADDR_ABP_R()
{
    switch (ins_step)
    {
    case 1:
        ins_stack.push_back(fetch(true));
        addr = ins_stack[1];
    default:
        return true;
    }
    return false;
}

uint8_t CPU::OP_ORA()
{
    uint8_t result = a | val;

    p = p & ~(FLG_ZRO | FLG_NEG);
    p |= (result == 0) ? FLG_ZRO : 0;
    p |= (result & (1 << 7)) ? FLG_NEG : 0;

    return result;
}

uint8_t CPU::OP_AND()
{
    uint8_t result = a & val;

    p = p & ~(FLG_ZRO | FLG_NEG);
    p |= (result == 0) ? FLG_ZRO : 0;
    p |= (result & (1 << 7)) ? FLG_NEG : 0;

    return result;
}

uint8_t CPU::OP_EOR()
{
    uint8_t result = a ^ val;

    p = p & ~(FLG_ZRO | FLG_NEG);
    p |= (result == 0) ? FLG_ZRO : 0;
    p |= (result & (1 << 7)) ? FLG_NEG : 0;

    return result;
}

uint8_t CPU::OP_ASL()
{
    p = p & ~(FLG_ZRO | FLG_NEG | FLG_CRY);
    p |= (val & 0x80) ? FLG_CRY : 0;

    uint8_t result = val << 1;

    p |= (result == 0) ? FLG_ZRO : 0;
    p |= (result & (1 << 7)) ? FLG_NEG : 0;

    return result;
}

uint8_t CPU::OP_LSR()
{
    p = p & ~(FLG_NEG | FLG_CRY | FLG_ZRO);
    p |= (val & 0x01) ? FLG_CRY : 0;

    uint8_t result = val >> 1;

    p |= (result == 0) ? FLG_ZRO : 0;

    return result;
}

uint8_t CPU::OP_ROL()
{

    uint8_t result = val << 1;
    result |= (last_p & FLG_CRY) ? 1 : 0;

    p = p & ~(FLG_ZRO | FLG_NEG | FLG_CRY);
    p |= (val & 0x80) ? FLG_CRY : 0;
    p |= (result == 0) ? FLG_ZRO : 0;
    p |= (result & (1 << 7)) ? FLG_NEG : 0;

    return result;
}

uint8_t CPU::OP_ROR()
{

    uint8_t result = val >> 1;
    result |= (last_p & FLG_CRY) ? (0x80) : 0;

    p = p & ~(FLG_ZRO | FLG_NEG | FLG_CRY);
    p |= (val & 1) ? FLG_CRY : 0;
    p |= (result == 0) ? FLG_ZRO : 0;
    p |= (result & (1 << 7)) ? FLG_NEG : 0;

    return result;
}

uint8_t CPU::OP_ADC()
{
    uint8_t carry = (FLG_CRY&last_p) ? 1 : 0;
    uint8_t result = a + val + carry;

    p = p & ~(FLG_ZRO | FLG_NEG | FLG_CRY | FLG_OVR);
    p |= (result == 0) ? FLG_ZRO : 0;
    p |= (result & 0x80) ? FLG_NEG : 0;
    p |= (result-carry < a) ? FLG_CRY : 0;
    p |= ((val ^ result) & (result ^ a))&0x80 ? FLG_OVR : 0;

    return result;
}

uint8_t CPU::OP_SBC()
{
    uint8_t val_comp = ~val;
    uint8_t borrow = (FLG_CRY&last_p) ? 1 : 0;
    uint8_t result = a + val_comp + borrow;

    p = p & ~(FLG_ZRO | FLG_NEG | FLG_CRY | FLG_OVR);
    p |= (result == 0) ? FLG_ZRO : 0;
    p |= (result & 0x80) ? FLG_NEG : 0;
    p |= (result-borrow < a) ? FLG_CRY : 0;
    p |= ((val_comp ^ result) & (result ^ a))&0x80 ? FLG_OVR : 0;

    return result;
}

uint8_t CPU::OP_DEC()
{
    uint8_t result = val - 1;

    p = p & ~(FLG_ZRO | FLG_NEG);
    p |= (result == 0) ? FLG_ZRO : 0;
    p |= (result & (1 << 7)) ? FLG_NEG : 0;

    return result;
}

uint8_t CPU::OP_INC()
{
    uint8_t result = val + 1;

    p = p & ~(FLG_ZRO | FLG_NEG);
    p |= (result == 0) ? FLG_ZRO : 0;
    p |= (result & (1 << 7)) ? FLG_NEG : 0;

    return result;
}

uint8_t CPU::OP_TST()
{
    p &= ~(FLG_NEG|FLG_OVR|FLG_ZRO);
    p |= (val&0x80) ? FLG_NEG : 0;
    p |= (val&0x40) ? FLG_OVR : 0;
    p |= ((a&val) == 0) ? FLG_ZRO : 0;

    return 0;
}

uint8_t CPU::OP_CMP(uint8_t cmpval)
{
    uint8_t res = cmpval - val;

    p &= ~(FLG_NEG|FLG_CRY|FLG_ZRO);
    p |= (res==0) ? FLG_ZRO : 0;
    p |= (res&0x80) ? FLG_NEG : 0;
    p |= (res <= cmpval) ? FLG_CRY : 0;

    return 0;
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

bool CPU::WB_PLA()
{
    switch(ins_step)
    {
    case 1:
        break;
    case 2:
        bus->get(0x0100+s++);
        break;
    case 3:
        a = bus->get(0x0100+s);
        p &= ~(FLG_NEG|FLG_ZRO);
        p |= (a==0) ? FLG_ZRO : 0;
        p |= (a&0x80) ? FLG_NEG : 0;
    default:
        return true;
    }
    return false;
}

bool CPU::WB_PLP()
{
    switch(ins_step)
    {
    case 1:
        break;
    case 2:
        bus->get(0x0100+s++);
        break;
    case 3:
        p &= 0x20;
        p |= bus->get(0x0100+s) & ~FLG_BRK;
    default:
        return true;
    }
    return false;
}

bool CPU::BRANCH()
{
    uint8_t pcl = (uint8_t)((uint8_t)(pc&0x00FF) + (int8_t)addr);
    switch(ins_step)
    {
    case 2:
        ins_stack.push_back(fetch(false));
        if ((((uint16_t)(pc + (int8_t)addr))&0xFF00) == (pc&0xFF00))
        {
            pc += (int8_t)addr;
            return true;
        }
        break;
    case 3:
        ins_stack.push_back(bus->get((pc&0xFF00) | pcl));
        pc += (int8_t)addr;
        return true;
    }
    return false;
}

bool CPU::WB_JSR()
{
    switch(ins_step)
    {
    case 1:
        break;
    case 2:
        bus->get(0x0100+s);
        break;
    case 3:
        bus->set(0x0100+s--, pc>>8);
        break;
    case 4:
        bus->set(0x0100+s--, (uint8_t)pc);
        break;
    case 5:
        addr |= fetch(true)<<8;
        pc = addr;
    default:
        return true;
    }
    return false;
}

bool CPU::WB_RTI()
{
    switch(ins_step)
    {
    case 1:
        break;
    case 2:
        bus->get(0x0100+s++);
        break;
    case 3:
        p &= 0x20;
        p |= bus->get(0x0100+s++) & ~FLG_BRK;
        break;
    case 4:
        pc = (pc&0xFF00) | bus->get(0x0100+s++);
        break;
    case 5:
        pc = (pc&0x00FF) | (bus->get(0x0100+s)<<8);
    default:
        return true;
    }
    return false;
}

bool CPU::WB_RTS()
{
    switch(ins_step)
    {
    case 1:
        break;
    case 2:
        bus->get(0x0100+s++);
        break;
    case 3:
        pc = (pc&0xFF00) | bus->get(0x0100+s++);
        break;
    case 4:
        pc = (pc&0x00FF) | (bus->get(0x0100+s)<<8);
        break;
    case 5:
        fetch(true);
    default:
        return true;
    }
    return false;
}

void CPU::OP_FLG(uint8_t flgval)
{
    p &= ~(FLG_NEG|FLG_ZRO);
    p |= (flgval==0) ? FLG_ZRO : 0;
    p |= (flgval&0x80) ? FLG_NEG : 0;
}

bool CPU::set_flag(uint8_t flag)
{
    p |= flag;
    return true;
}

bool CPU::clear_flag(uint8_t flag)
{
    p &= ~flag;
    return true;
}
