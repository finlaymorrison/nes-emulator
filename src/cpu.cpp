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
    pc{}, a{}, x{}, y{}, s{}, p{}, bus{nullptr}, ins_step{-1}, last_p{}
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
    return ins_step >= 0;
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
    if (wb_cycle == 0)
    {
        wb_cycle++;
        return false;
    }
    if (wb_cycle == 1 && delay)
    {
        bus->set(addr, val);
        wb_cycle++;
        return false;
    }
    bus->set(addr, comp_val);
    return true;
}

void CPU::clock_cycle()
{
    ++ins_step;
    if (ins_step == 0)
    {
        opcode = fetch(true);
        last_p = p;
        addr = 0;
        buf = 0;
        val = 0;
        wb_cycle = 0;
        return;
    }

    switch (opcode)
    {
    case 0x00: // BRK impl
        if (!ADDR_IMP()) return;
        if (!WB_BRK()) return;
        break;
    case 0x01: // ORA X,ind
        if (!ADDR_INX()) return;
        a = OP_ORA();
        break;
    case 0x05: // ORA zpg
        if (!ADDR_ZP()) return;
        a = OP_ORA();
        break;
    case 0x06: // ASL zpg
        if (!ADDR_ZP()) return;
        if (!WB_MEM(OP_ASL())) return;
        break;
    case 0x08: // PHP impl
        if (!ADDR_IMP()) return;
        if (!WB_PHP()) return;
        break;
    case 0x09: // ORA #
        if (!ADDR_IM()) return;
        a = OP_ORA();
        break;
    case 0x0A: // ASL A
        if (!ADDR_IMP()) return;
        a = OP_ASL();
        break;
    case 0x0D: // ORA abs
        if (!ADDR_AB()) return;
        a = OP_ORA();
        break;
    case 0x0E: // ASL abs
        if (!ADDR_AB()) return;
        if (!WB_MEM(OP_ASL())) return;
        break;
    case 0x10: // BPL rel
        if (!ADDR_REL()) return;
        if (FLG_NEG & p) break;
        if (!BRANCH()) return;
        break;
    case 0x11: // ORA ind,y
        if (!ADDR_INY()) return;
        a = OP_ORA();
        break;
    case 0x15: // ORA zpg,x
        if (!zeropage_indexed(x)) return;
        a = OP_ORA();
        break;
    case 0x16: // ASL zpg,x
        if (!zeropage_indexed(x)) return;
        if (!WB_MEM(OP_ASL())) return;
        break;
    case 0x18: // CLC impl
        if (!ADDR_IMP()) return;
        clear_flag(FLG_CRY);
        break;
    case 0x19: // ORA abs,Y
        if (!absolute_indexed(y, true)) return;
        a = OP_ORA();
        break;
    case 0x1D: // ORA abs,X
        if (!absolute_indexed(x, true)) return;
        a = OP_ORA();
        break;
    case 0x1E: // ASL abs,X
        if (!absolute_indexed(x, true, false)) return;
        if (!WB_MEM(OP_ASL())) return;
        break;
    case 0x20: // JSR abs
        if (!ADDR_ABP_R()) return;
        if (!WB_JSR()) return;
        break;
    case 0x21: // AND X,ind
        if (!ADDR_INX()) return;
        a = OP_AND();
        break;
    case 0x24: // BIT zpg
        if (!ADDR_ZP()) return;
        OP_TST();
        break;
    case 0x25: // AND zpg
        if (!ADDR_ZP()) return;
        a = OP_AND();
        break;
    case 0x26: // ROL zpg
        if (!ADDR_ZP()) return;
        if (!WB_MEM(OP_ROL())) return;
        break;
    case 0x28: // PLP impl
        if (!ADDR_IMP()) return;
        if (!WB_PLP()) return;
        break;
    case 0x29: // AND #
        if (!ADDR_IM()) return;
        a = OP_AND();
        break;
    case 0x2A: // ROL A
        if (!ADDR_IMP()) return;
        a = OP_ROL();
        break;
    case 0x2C: // BIT abs
        if (!ADDR_AB()) return;
        OP_TST();
        break;
    case 0x2D: // AND abs
        if (!ADDR_AB()) return;
        a = OP_AND();
        break;
    case 0x2E: // ROL abs
        if (!ADDR_AB()) return;
        if (!WB_MEM(OP_ROL())) return;
        break;
    case 0x30: // BMI rel
        if (!ADDR_REL()) return;
        if (!(FLG_NEG & p)) break;
        if (!BRANCH()) return;
        break;
    case 0x31: // AND ind,Y
        if (!ADDR_INY()) return;
        a = OP_AND();
        break;
    case 0x35: // AND zpg,X
        if (!zeropage_indexed(x)) return;
        a = OP_AND();
        break;
    case 0x36: // ROL zpg,X
        if (!zeropage_indexed(x)) return;
        if (!WB_MEM(OP_ROL())) return;
        break;
    case 0x38: // SEC impl
        if (!ADDR_IMP()) return;
        set_flag(FLG_CRY);
        break;
    case 0x39: // AND abs,Y
        if (!absolute_indexed(y, true)) return;
        a = OP_AND();
        break;
    case 0x3D: // AND abs,X
        if (!absolute_indexed(x, true)) return;
        a = OP_AND();
        break;
    case 0x3E: // ROL abs,X
        if (!absolute_indexed(x, true, false)) return;
        if (!WB_MEM(OP_ROL())) return;
        break;
    case 0x40: // RTI impl
        if (!ADDR_IMP()) return;
        if (!WB_RTI()) return;
        break;
    case 0x41: // EOR X,ind
        if (!ADDR_INX()) return;
        a = OP_EOR();
        break;
    case 0x45: // EOR zpg
        if (!ADDR_ZP()) return;
        a = OP_EOR();
        break;
    case 0x46: // LSR zpg
        if (!ADDR_ZP()) return;
        if (!WB_MEM(OP_LSR())) return;
        break;
    case 0x48: // PHA impl
        if (!ADDR_IMP()) return;
        if (!WB_PHA()) return;
        break;
    case 0x49: // EOR #
        if (!ADDR_IM()) return;
        a = OP_EOR();
        break;
    case 0x4A: // LSR A
        if (!ADDR_IMP()) return;
        a = OP_LSR();
        break;
    case 0x4C: // JMP abs
        if (!ADDR_AB_R()) return;
        pc = addr;
        break;
    case 0x4D: // EOR abs
        if (!ADDR_AB()) return;
        a = OP_EOR();
        break;
    case 0x4E: // LSR abs
        if (!ADDR_AB()) return;
        if (!WB_MEM(OP_LSR())) return;
        break;
    case 0x50: // BVC rel
        if (!ADDR_REL()) return;
        if (FLG_OVR & p) break;
        if (!BRANCH()) return;
        break;
    case 0x51: // EOR ind,Y
        if (!ADDR_INY()) return;
        a = OP_EOR();
        break;
    case 0x55: // EOR zpg,X
        if (!zeropage_indexed(x)) return;
        a = OP_EOR();
        break;
    case 0x56: // LSR zpg,X
        if (!zeropage_indexed(x)) return;
        if (!WB_MEM(OP_LSR())) return;
        break;
    case 0x58: // CLI impl
        if (!ADDR_IMP()) return;
        clear_flag(FLG_INT);
        break;
    case 0x59: // EOR abs,Y
        if (!absolute_indexed(y, true)) return;
        a = OP_EOR();
        break;
    case 0x5D: // EOR abs,X
        if (!absolute_indexed(x, true)) return;
        a = OP_EOR();
        break;
    case 0x5E: // LSR abs,X
        if (!absolute_indexed(x, true, false)) return;
        if (!WB_MEM(OP_LSR())) return;
        break;
    case 0x60: // RTS impl
        if (!ADDR_IMP()) return;
        if (!WB_RTS()) return;
        break;
    case 0x61: // ADC X,ind
        if (!ADDR_INX()) return;
        a = OP_ADC();
        break;
    case 0x65: // ADC zpg
        if (!ADDR_ZP()) return;
        a = OP_ADC();
        break;
    case 0x66: // ROR zpg
        if (!ADDR_ZP()) return;
        if (!WB_MEM(OP_ROR())) return;
        break;
    case 0x68: // PLA impl
        if (!ADDR_IMP()) return;
        if (!WB_PLA()) return;
        break;
    case 0x69: // ADC #
        if (!ADDR_IM()) return;
        a = OP_ADC();
        break;
    case 0x6A: // ROR A
        if (!ADDR_IMP()) return;
        a = OP_ROR();
        break;
    case 0x6C: // JMP ind
        if (!ADDR_IN_R()) return;
        pc = addr;
        break;
    case 0x6D: // ADC abs
        if (!ADDR_AB()) return;
        a = OP_ADC();
        break;
    case 0x6E: // ROR abs
        if (!ADDR_AB()) return;
        if (!WB_MEM(OP_ROR())) return;
        break;
    case 0x70: // BVS rel
        if (!ADDR_REL()) return;
        if (!(FLG_OVR & p)) break;
        if (!BRANCH()) return;
        break;
    case 0x71: // ADC ind,Y
        if (!ADDR_INY()) return;
        a = OP_ADC();
        break;
    case 0x75: // ADC zpg,X
        if (!zeropage_indexed(x)) return;
        a = OP_ADC();
        break;
    case 0x76: // ROR zpg,X
        if (!zeropage_indexed(x)) return;
        if (!WB_MEM(OP_ROR())) return;
        break;
    case 0x78: // SEI impl
        if (!ADDR_IMP()) return;
        set_flag(FLG_INT);
        break;
    case 0x79: // ADC abs,Y
        if (!absolute_indexed(y, true)) return;
        a = OP_ADC();
        break;
    case 0x7D: // ADC abs,X
        if (!absolute_indexed(x, true)) return;
        a = OP_ADC();
        break;
    case 0x7E: // ROR abs,X
        if (!absolute_indexed(x, true, false)) return;
        if (!WB_MEM(OP_ROR())) return;;
        break;
    case 0x81: // STA X,ind
        if (!ADDR_INX_R()) return;
        if (!WB_MEM(a, false)) return;
        break;
    case 0x84: // STY zpg
        if (!ADDR_ZP_R()) return;
        if (!WB_MEM(y, false)) return;
        break;
    case 0x85: // STA zpg
        if (!ADDR_ZP_R()) return;
        if (!WB_MEM(a, false)) return;
        break;
    case 0x86: // STX zpg
        if (!ADDR_ZP_R()) return;
        if (!WB_MEM(x, false)) return;
        break;
    case 0x88: // DEY impl
        if (!ADDR_IMP()) return;
        val = y;
        y = OP_DEC();
        break;
    case 0x8A: // TXA impl
        if (!ADDR_IMP()) return;
        OP_FLG(x);
        a = x;
        break;
    case 0x8C: // STY abs
        if (!ADDR_AB_R()) return;
        if (!WB_MEM(y, false)) return;
        break;
    case 0x8D: // STA abs
        if (!ADDR_AB_R()) return;
        if (!WB_MEM(a, false)) return;
        break;
    case 0x8E: // STX abs
        if (!ADDR_AB_R()) return;
        if (!WB_MEM(x, false)) return;
        break;
    case 0x90: // BCC rel
        if (!ADDR_REL()) return;
        if (FLG_CRY & p) break;
        if (!BRANCH()) return;
        break;
    case 0x91: // STA ind,Y
        if (!ADDR_INY_R(false)) return;
        if (!WB_MEM(a, false)) return;
        break;
    case 0x94: // STY zpg,X
        if (!zeropage_indexed(x, false)) return;
        if (!WB_MEM(y, false)) return;
        break;
    case 0x95: // STA zpg,X
        if (!zeropage_indexed(x, false)) return;
        if (!WB_MEM(a, false)) return;
        break;
    case 0x96: // STX zpg,y
        if (!zeropage_indexed(y, false)) return;
        if (!WB_MEM(x, false)) return;
        break;
    case 0x98: // TYA impl
        if (!ADDR_IMP()) return;
        OP_FLG(y);
        a = y;
        break;
    case 0x99: // STA abs,Y
        if (!absolute_indexed(y, false, false)) return;
        if (!WB_MEM(a, false)) return;
        break;
    case 0x9A: // TXS impl
        if (!ADDR_IMP()) return;
        s = x;
        break;
    case 0x9D: // STA abs,X
        if (!absolute_indexed(x, false, false)) return;
        if (!WB_MEM(a, false)) return;
        break;
    case 0xA0: // LDY #
        if (!ADDR_IM()) return;
        OP_FLG(val);
        y = val;
        break;
    case 0xA1: // LDA X,ind
        if (!ADDR_INX()) return;
        OP_FLG(val);
        a = val;
        break;
    case 0xA2: // LDX #
        if (!ADDR_IM()) return;
        OP_FLG(val);
        x = val;
        break;
    case 0xA4: // LDY zpg
        if (!ADDR_ZP()) return;
        OP_FLG(val);
        y = val;
        break;
    case 0xA5: // LDA zpg
        if (!ADDR_ZP()) return;
        OP_FLG(val);
        a = val;
        break;
    case 0xA6: // LDX zpg
        if (!ADDR_ZP()) return;
        OP_FLG(val);
        x = val;
        break;
    case 0xA8: // TAY impl
        if (!ADDR_IMP()) return;
        OP_FLG(a);
        y = a;
        break;
    case 0xA9: // LDA #
        if (!ADDR_IM()) return;
        OP_FLG(val);
        a = val;
        break;
    case 0xAA: // TAX impl
        if (!ADDR_IMP()) return;
        OP_FLG(a);
        x = a;
        break;
    case 0xAC: // LDY abs
        if (!ADDR_AB()) return;
        OP_FLG(val);
        y = val;
        break;
    case 0xAD: // LDA abs
        if (!ADDR_AB()) return;
        OP_FLG(val);
        a = val;
        break;
    case 0xAE: // LDX abs
        if (!ADDR_AB()) return;
        OP_FLG(val);
        x = val;
        break;
    case 0xB0: // BCS rel
        if (!ADDR_REL()) return;
        if (!(FLG_CRY & p)) break;
        if (!BRANCH()) return;
        break;
    case 0xB1: // LDA ind,Y
        if (!ADDR_INY()) return;
        OP_FLG(val);
        a = val;
        break;
    case 0xB4: // LDY zpg,X
        if (!zeropage_indexed(x)) return;
        OP_FLG(val);
        y = val;
        break;
    case 0xB5: // LDA zpg,X
        if (!zeropage_indexed(x)) return;
        OP_FLG(val);
        a = val;
        break;
    case 0xB6: // LDX zpg,Y
        if (!zeropage_indexed(y)) return;
        OP_FLG(val);
        x = val;
        break;
    case 0xB8: // CLV impl
        if (!ADDR_IMP()) return;
        clear_flag(FLG_OVR);
        break;
    case 0xB9: // LDA abs,Y
        if (!absolute_indexed(y, true)) return;
        OP_FLG(val);
        a = val;
        break;
    case 0xBA: // TSX impl
        if (!ADDR_IMP()) return;
        OP_FLG(s);
        x = s;
        break;
    case 0xBC: // LDY abs,X
        if (!absolute_indexed(x, true)) return;
        OP_FLG(val);
        y = val;
        break;
    case 0xBD: // LDA abs,X
        if (!absolute_indexed(x, true)) return;
        OP_FLG(val);
        a = val;
        break;
    case 0xBE: // LDX abs,Y
        if (!absolute_indexed(y, true)) return;
        OP_FLG(val);
        x = val;
        break;
    case 0xC0: // CPY #
        if (!ADDR_IM()) return;
        OP_CMP(y);
        break;
    case 0xC1: // CMP X,ind
        if (!ADDR_INX()) return;
        OP_CMP(a);
        break;
    case 0xC4: // CPY zpg
        if (!ADDR_ZP()) return;
        OP_CMP(y);
        break;
    case 0xC5: // CMP zpg
        if (!ADDR_ZP()) return;
        OP_CMP(a);
        break;
    case 0xC6: // DEC zpg
        if (!ADDR_ZP()) return;
        if (!WB_MEM(OP_DEC())) return;
        break;
    case 0xC8: // INY impl
        if (!ADDR_IMP()) return;
        val = y;
        y = OP_INC();
        break;
    case 0xC9: // CMP #
        if (!ADDR_IM()) return;
        OP_CMP(a);
        break;
    case 0xCA: // DEX impl
        if (!ADDR_IMP()) return;
        val = x;
        x = OP_DEC();
        break;
    case 0xCC: // CPY abs
        if (!ADDR_AB()) return;
        OP_CMP(y);
        break;
    case 0xCD: // CMP abs
        if (!ADDR_AB()) return;
        OP_CMP(a);
        break;
    case 0xCE: // DEC abs
        if (!ADDR_AB()) return;
        if (!WB_MEM(OP_DEC())) return;
        break;
    case 0xD0: // BNE rel
        if (!ADDR_REL()) return;
        if (FLG_ZRO & p) break;
        if (!BRANCH()) return;
        break;
    case 0xD1: // CMP ind,Y
        if (!ADDR_INY()) return;
        OP_CMP(a);
        break;
    case 0xD5: // CMP zpg,X
        if (!zeropage_indexed(x)) return;
        OP_CMP(a);
        break;
    case 0xD6: // DEC zpg,X
        if (!zeropage_indexed(x)) return;
        if (!WB_MEM(OP_DEC())) return;
        break;
    case 0xD9: // CMP abs,Y
        if (!absolute_indexed(y, true)) return;
        OP_CMP(a);
        break;
    case 0xDD: // CMP abs,X
        if (!absolute_indexed(x, true)) return;
        OP_CMP(a);
        break;
    case 0xDE: // DEC abs,X
        if (!absolute_indexed(x, true, false)) return;
        if (!WB_MEM(OP_DEC())) return;
        break;
    case 0xE0: // CPX #
        if (!ADDR_IM()) return;
        OP_CMP(x);
        break;
    case 0xE1: // SBC X,ind
        if (!ADDR_INX()) return;
        a = OP_SBC();
        break;
    case 0xE4: // CPX zpg
        if (!ADDR_ZP()) return;
        OP_CMP(x);
        break;
    case 0xE5: // SBC zpg
        if (!ADDR_ZP()) return;
        a = OP_SBC();
        break;
    case 0xE6: // INC zpg
        if (!ADDR_ZP()) return;
        if (!WB_MEM(OP_INC())) return;
        break;
    case 0xE8: // INX impl
        if (!ADDR_IMP()) return;
        val = x;
        x = OP_INC();
        break;
    case 0xE9: // SBC #
        if (!ADDR_IM()) return;
        a = OP_SBC();
        break;
    case 0xEA: // NOP impl
        if (!ADDR_IMP()) return;
        break;
    case 0xEC: // CPX abs
        if (!ADDR_AB()) return;
        OP_CMP(x);
        break;
    case 0xED: // SBC abs
        if (!ADDR_AB()) return;
        a = OP_SBC();
        break;
    case 0xEE: // INC abs
        if (!ADDR_AB()) return;
        if (!WB_MEM(OP_INC())) return;
        break;
    case 0xF0: // BEQ rel
        if (!ADDR_REL()) return;
        if (!(FLG_ZRO & p)) break;
        if (!BRANCH()) return;
        break;
    case 0xF1: // SBC ind,Y
        if (!ADDR_INY()) return;
        a = OP_SBC();
        break;
    case 0xF5: // SBC zpg,X
        if (!zeropage_indexed(x)) return;
        a = OP_SBC();
        break;
    case 0xF6: // INC zpg,X
        if (!zeropage_indexed(x)) return;
        if (!WB_MEM(OP_INC())) return;
        break;
    case 0xF9: // SBC abs,Y
        if (!absolute_indexed(y, true)) return;
        a = OP_SBC();
        break;
    case 0xFD: // SBC abs,X
        if (!absolute_indexed(x, true)) return;
        a = OP_SBC();
        break;
    case 0xFE: // INC abs,X
        if (!absolute_indexed(x, true, false)) return;
        if (!WB_MEM(OP_INC())) return;
        break;
    default:
        break;
    }

    ins_step = -1;
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

bool CPU::zeropage_indexed(uint8_t index, bool resolve)
{
    switch (ins_step)
    {
    case 1:
        addr = fetch(true);
        break;
    case 2:
        bus->get(addr);
        addr = static_cast<uint8_t>(addr + index);
        if (!resolve) return true;
        break;
    case 3:
        if (resolve) val = bus->get(addr);
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
        addr |= fetch(true) << 8;
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

bool CPU::absolute_indexed(uint8_t index, bool resolve, bool optimise)
{
    switch (ins_step)
    {
    case 1:
        buf = fetch(true);
        break;
    case 2:
        buf |= fetch(true)<<8;
        if (!resolve)
        {
            if ((uint8_t)(buf + index) >= index && optimise)
            {
                addr = buf + index;
                return true;
            }
        }
        break;
    case 3:
        addr = buf&0xFF00 | static_cast<uint8_t>(buf+index);
        if (!resolve)
        {
            if ((uint8_t)(buf + index) < index || !optimise)
            {
                bus->get(addr);
                addr = buf + index;
                return true;
            }
        }
        else
        {
            val = bus->get(addr);
            if (static_cast<uint8_t>(buf + index) >= index && optimise)
            {
                return true;
            }
        }
        break;
    case 4:
        if (!resolve)
        {
            return true;
        }
        else
        {
            if (static_cast<uint8_t>(buf + index) < index || !optimise)
            {
                addr = buf + index;
                val = bus->get(addr);
            }
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
        buf = fetch(true);
        break;
    case 2:
        bus->get(buf);
        buf = static_cast<uint8_t>(buf + x);
        break;
    case 3:
    case 4:
        addr >>= 8;
        addr |= bus->get(buf&0xFF)<<8;
        buf += 1;
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
        buf >>= 8;
        buf |= fetch(true)<<8;
        break;
    case 3:
        addr = bus->get(buf);
        break;
    case 4:
        addr |= bus->get(buf&0xFF00 | static_cast<uint8_t>(buf+1))<<8;
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
        addr = fetch(true);
        break;
    case 2:
    case 3:
        buf >>= 8;
        buf |= bus->get(static_cast<uint8_t>(addr++))<<8;
        break;
    case 4:
        addr = (buf&0xFF00) | static_cast<uint8_t>(buf+y);
        val = bus->get(addr);
        if (static_cast<uint8_t>(buf+y) >= y) return true;
        break;
    case 5:
        if (static_cast<uint8_t>(buf+y) < y)
        {
            addr = buf + y;
            val = bus->get(addr);
        }
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
        buf = fetch(true);
        break;
    case 2:
        bus->get(buf);
        buf = static_cast<uint8_t>(buf + x);
        break;
    case 3:
        addr = bus->get(buf);
        break;
    case 4:
        addr |= bus->get(static_cast<uint8_t>(buf+1))<<8;
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
        addr = fetch(true);
        break;
    case 2:
        buf = bus->get(addr);
        break;
    case 3:
        buf |= bus->get(static_cast<uint8_t>(addr+1))<<8;
        addr = (buf&0xFF00) | static_cast<uint8_t>(buf+y);
        if (static_cast<uint8_t>(buf&0xFF+y) >= y && optimise) return true;
        break;
    case 4:
        if (static_cast<uint8_t>(buf&0xFF+y) < y || !optimise)
        {
            bus->get(addr);
            addr = buf + y;
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
        addr = fetch(true);
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
        addr = fetch(true);
    default:
        return true;
    }
    return false;
}

uint8_t CPU::OP_ORA()
{
    uint8_t result = a | val;

    update_flag(FLG_NEG, result & 0x80);
    update_flag(FLG_ZRO, result == 0);

    return result;
}

uint8_t CPU::OP_AND()
{
    uint8_t result = a & val;

    update_flag(FLG_NEG, result & 0x80);
    update_flag(FLG_ZRO, result == 0);

    return result;
}

uint8_t CPU::OP_EOR()
{
    uint8_t result = a ^ val;

    update_flag(FLG_NEG, result & 0x80);
    update_flag(FLG_ZRO, result == 0);

    return result;
}

uint8_t CPU::OP_ASL()
{
    update_flag(FLG_CRY, val & 0x80);

    uint8_t result = val << 1;

    update_flag(FLG_NEG, result & 0x80);
    update_flag(FLG_ZRO, result == 0);

    return result;
}

uint8_t CPU::OP_LSR()
{
    update_flag(FLG_CRY, val & 0x1);

    uint8_t result = val >> 1;

    update_flag(FLG_ZRO, result == 0);
    clear_flag(FLG_NEG);

    return result;
}

uint8_t CPU::OP_ROL()
{

    uint8_t result = val << 1;
    result |= (last_p & FLG_CRY) ? 1 : 0;

    update_flag(FLG_NEG, result & 0x80);
    update_flag(FLG_ZRO, result == 0);
    update_flag(FLG_CRY, val & 0x80);

    return result;
}

uint8_t CPU::OP_ROR()
{

    uint8_t result = val >> 1;
    result |= (last_p & FLG_CRY) ? (0x80) : 0;

    update_flag(FLG_NEG, result & 0x80);
    update_flag(FLG_ZRO, result == 0);
    update_flag(FLG_CRY, val & 0x1);

    return result;
}

uint8_t CPU::OP_ADC()
{
    uint8_t carry = (FLG_CRY&last_p) ? 1 : 0;
    uint8_t result = a + val + carry;

    update_flag(FLG_NEG, result & 0x80);
    update_flag(FLG_ZRO, result == 0);
    update_flag(FLG_CRY, result-carry < a);
    update_flag(FLG_OVR, ((val ^ result) & (result ^ a))&0x80);

    return result;
}

uint8_t CPU::OP_SBC()
{
    uint8_t val_comp = ~val;
    uint8_t borrow = (FLG_CRY&last_p) ? 1 : 0;
    uint8_t result = a + val_comp + borrow;

    update_flag(FLG_NEG, result & 0x80);
    update_flag(FLG_ZRO, result == 0);
    update_flag(FLG_CRY, result-borrow < a);
    update_flag(FLG_OVR, ((val_comp ^ result) & (result ^ a))&0x80);

    return result;
}

uint8_t CPU::OP_DEC()
{
    uint8_t result = val - 1;

    update_flag(FLG_NEG, result & 0x80);
    update_flag(FLG_ZRO, result == 0);

    return result;
}

uint8_t CPU::OP_INC()
{
    uint8_t result = val + 1;

    update_flag(FLG_NEG, result & 0x80);
    update_flag(FLG_ZRO, result == 0);

    return result;
}

uint8_t CPU::OP_TST()
{
    update_flag(FLG_NEG, val & 0x80);
    update_flag(FLG_OVR, val & 0x40);
    update_flag(FLG_ZRO, (a & val) == 0);

    return 0;
}

uint8_t CPU::OP_CMP(uint8_t cmpval)
{
    uint8_t res = cmpval - val;

    update_flag(FLG_ZRO, res == 0);
    update_flag(FLG_NEG, res & 0x80);
    update_flag(FLG_CRY, res <= cmpval);

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
        fetch(false);
        if ((((uint16_t)(pc + (int8_t)addr))&0xFF00) == (pc&0xFF00))
        {
            pc += (int8_t)addr;
            return true;
        }
        break;
    case 3:
        bus->get((pc&0xFF00) | pcl);
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
    update_flag(FLG_ZRO, flgval == 0);
    update_flag(FLG_NEG, flgval & 0x80);
}

void CPU::update_flag(uint8_t flag, bool condition)
{
    p = (p & ~flag) | (-static_cast<int>(condition) & flag);
}

void CPU::set_flag(uint8_t flag)
{
    p |= flag;
}

void CPU::clear_flag(uint8_t flag)
{
    p &= ~flag;
}
