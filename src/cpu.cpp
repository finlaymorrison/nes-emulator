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
    pc{}, a{}, x{}, y{}, s{}, p{}, bus{nullptr}, ins_stack{}, ins_step{0}
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
    bool valid = true;

    std::cerr << std::dec;
    if (json["pc"] != pc) {
        std::cerr << "State mismatch in PC: Expected " << json["pc"] << ", got " << (int)pc << std::endl;
        valid = false;
    }

    if (json["a"] != a) {
        std::cerr << "State mismatch in A: Expected " << json["a"] << ", got " << (int)a << std::endl;
        valid = false;
    }

    if (json["x"] != x) {
        std::cerr << "State mismatch in X: Expected " << json["x"] << ", got " << (int)x << std::endl;
        valid = false;
    }

    if (json["y"] != y) {
        std::cerr << "State mismatch in Y: Expected " << json["y"] << ", got " << (int)y << std::endl;
        valid = false;
    }

    if (json["s"] != s) {
        std::cerr << "State mismatch in S: Expected " << json["s"] << ", got " << (int)s << std::endl;
        valid = false;
    }

    if (json["p"] != p) {
        std::cerr << "State mismatch in P: Expected " << json["p"] << ", got " << (int)p << std::endl;
        valid = false;
    }

    return valid;
}

uint8_t CPU::fetch(bool inc)
{
    uint8_t byte = bus->get(pc);
    if (inc) ++pc;
    return byte;
}

void CPU::clock_cycle()
{
    if (ins_stack.size() == 0)
    {
        ins_stack.push_back(fetch(true));
    }

    bool complete;
    switch (ins_stack[0])
    {
        case 0xEA:
            complete = OP_NOP_IMP();
        break;
        case 0x29:
            complete = OP_AND_IM();
        break;
        case 0x25:
            complete = OP_AND_ZP();
        break;
        case 0x35:
            complete = OP_AND_ZPX();
        break;
        case 0x2D:
            complete = OP_AND_AB();
        break;
        default:
            complete = true;
            std::cerr << "Unknown opcode : 0x" << std::uppercase
                << std::hex << (int)ins_stack[0] << std::endl;
    }

    if (complete)
    {
        ins_step = 0;
        ins_stack.clear();
    }
    else
    {
        ++ins_step;
    }
}

bool CPU::OP_NOP_IMP()
{
    if (ins_step == 1)
    {
        fetch(false);
        return true;
    }
    return false;
}

bool CPU::OP_AND_IM()
{
    if (ins_step == 1)
    {
        ins_stack.push_back(fetch(true));
        a &= ins_stack.back();
        p = (p & ~(FLG_ZRO|FLG_NEG)) |
            ((a == 0) ? FLG_ZRO : 0) |
            ((a & 0x80) ? FLG_NEG : 0);
        return true;
    }
    return false;
}

bool CPU::OP_AND_ZP()
{
    if (ins_step == 1)
    {
        ins_stack.push_back(fetch(true));
    }
    else if (ins_step == 2)
    {
        ins_stack.push_back(bus->get(ins_stack[1]));
        a &= ins_stack.back();
        p = (p & ~(FLG_ZRO|FLG_NEG)) |
            ((a == 0) ? FLG_ZRO : 0) |
            ((a & 0x80) ? FLG_NEG : 0);
        return true;
    }
    return false;
}

bool CPU::OP_AND_ZPX()
{
    if (ins_step == 1)
    {
        ins_stack.push_back(fetch(true));
    }
    else if (ins_step == 2)
    {
        bus->get(ins_stack.back());
        ins_stack.back() += x;
    }
    else if (ins_step == 3)
    {
        ins_stack.push_back(bus->get(ins_stack.back()));
        a &= ins_stack.back();
        p = (p & ~(FLG_ZRO|FLG_NEG)) |
            ((a == 0) ? FLG_ZRO : 0) |
            ((a & 0x80) ? FLG_NEG : 0);
        return true;
    }
    return false;
}

bool CPU::OP_AND_AB()
{
    if (ins_step == 1 || ins_step == 2)
    {
        ins_stack.push_back(fetch(true));
    }
    if (ins_step == 3)
    {
        ins_stack.push_back(bus->get(((uint16_t)ins_stack[2]<<8) | ins_stack[1]));
        a &= ins_stack.back();
        p = (p & ~(FLG_ZRO|FLG_NEG)) |
            ((a == 0) ? FLG_ZRO : 0) |
            ((a & 0x80) ? FLG_NEG : 0);
        return true;
    }
    return false;
}