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
        ++ins_step;
        return;
    }

    bool complete;
    switch ((ins_stack[0]))
    {
    case 0x00:
        // BRK impl
        complete = true;
        break;
    case 0x01:
        // ORA X,ind
        complete = true;
        break;
    case 0x05:
        complete = ADDR_ZP();
        if (complete) complete = OP_OR();
        break;
    case 0x09:
        complete = ADDR_IM();
        if (complete) complete = OP_OR();
        break;
    case 0x0D:
        complete = ADDR_AB();
        if (complete) complete = OP_OR();
        break;
    case 0x25:
        complete = ADDR_ZP();
        if (complete) complete = OP_AND();
        break;
    case 0x21:
        complete = ADDR_INX();
        if (complete) complete = OP_AND();
        break;
    case 0x29:
        complete = ADDR_IM();
        if (complete) complete = OP_AND();
        break;
    case 0x2D:
        complete = ADDR_AB();
        if (complete) complete = OP_AND();
        break;
    case 0x31:
        complete = ADDR_INY();
        if (complete) complete = OP_AND();
        break;
    case 0x35:
        complete = ADDR_ZPX();
        if (complete) complete = OP_AND();
        break;
    case 0x39:
        complete = ADDR_ABY();
        if (complete) complete = OP_AND();
        break;
    case 0x3D:
        complete = ADDR_ABX();
        if (complete) complete = OP_AND();
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
    ins_stack.clear();
}

bool CPU::ADDR_IM()
{
    if (ins_step == 1)
    {
        ins_stack.push_back(fetch(true));
    }
    else
    {
        return true;
    }
    return false;
}

bool CPU::ADDR_ZP()
{
    if (ins_step == 1)
    {
        ins_stack.push_back(fetch(true));
    }
    else if (ins_step == 2)
    {
        ins_stack.push_back(bus->get(ins_stack.back()));
    }
    else
    {
        return true;
    }
    return false;
}

bool CPU::ADDR_ZPX()
{
    switch (ins_step)
    {
    case 1:
        ins_stack.push_back(fetch(true));
        break;
    case 2:
        bus->get(ins_stack[1]);
        ins_stack[1] += x;
        break;
    case 3:
        ins_stack.push_back(bus->get(ins_stack[1]));
    default:
        return true;
    }
    return false;
}

bool CPU::ADDR_AB()
{
    if (ins_step == 1 || ins_step == 2)
    {
        ins_stack.push_back(fetch(true));
    }
    else
    {
        uint16_t addr = (uint16_t)ins_stack[2]<<8 | ins_stack[1];
        ins_stack.push_back(bus->get(addr));
        return true;
    }
    return false;
}

bool CPU::ADDR_INX()
{
    switch (ins_step)
    {
    case 1:
        ins_stack.push_back(fetch(true));
        break;
    case 2:
        bus->get(ins_stack[1]);
        ins_stack.back() += x;
        break;
    case 3:
    case 4:
        ins_stack.push_back(bus->get(ins_stack[1]++));
        break;
    case 5:{
        uint16_t addr = (static_cast<uint16_t>(ins_stack[3])<<8) | ins_stack[2];
        ins_stack.push_back(bus->get(addr));
        }
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
    case 4:{
        uint16_t addr = (static_cast<uint16_t>(ins_stack[3])<<8) | (uint8_t)(ins_stack[2]+y);
        ins_stack.push_back(bus->get(addr));
        if ((uint8_t)(ins_stack[2]+y) > y) return true;
        }break;
    case 5:{
        uint16_t init_addr = (static_cast<uint16_t>(ins_stack[3])<<8) | (uint8_t)(ins_stack[2]+y);
        if ((uint8_t)(ins_stack[2]+y) < y)
        {
            uint16_t true_addr = (static_cast<uint16_t>(ins_stack[3])<<8) + ins_stack[2] + y;
            ins_stack.push_back(bus->get(true_addr));
        }}
    default:
        return true;
    }
    return false;
}

bool CPU::ADDR_ABX()
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
        if ((uint8_t)(ins_stack[1]+x) > x) return true;
        }break;
    case 4:{
        uint16_t init_addr = (static_cast<uint16_t>(ins_stack[2])<<8) | (uint8_t)(ins_stack[1]+x);
        if ((uint8_t)(ins_stack[1]+x) < x)
        {
            uint16_t true_addr = (static_cast<uint16_t>(ins_stack[2])<<8) + ins_stack[1] + x;
            ins_stack.push_back(bus->get(true_addr));
        }}
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
        ins_stack.push_back(fetch(true));
        break;
    case 3:{
        uint16_t addr = (static_cast<uint16_t>(ins_stack[2])<<8) | (uint8_t)(ins_stack[1]+y);
        ins_stack.push_back(bus->get(addr));
        if ((uint8_t)(ins_stack[1]+y) > y) return true;
        }break;
    case 4:{
        uint16_t init_addr = (static_cast<uint16_t>(ins_stack[2])<<8) | (uint8_t)(ins_stack[1]+y);
        if ((uint8_t)(ins_stack[1]+y) < y)
        {
            uint16_t true_addr = (static_cast<uint16_t>(ins_stack[2])<<8) + ins_stack[1] + y;
            ins_stack.push_back(bus->get(true_addr));
        }}
    default:
        return true;
    }
    return false;
}

bool CPU::OP_OR()
{
    a = a | ins_stack.back();

    p = p & ~(FLG_ZRO | FLG_NEG);
    p |= (a == 0) ? FLG_ZRO : 0;
    p |= (a & (1 << 7)) ? FLG_NEG : 0;

    return true;
}

bool CPU::OP_AND()
{
    a = a & ins_stack.back();

    p = p & ~(FLG_ZRO | FLG_NEG);
    p |= (a == 0) ? FLG_ZRO : 0;
    p |= (a & (1 << 7)) ? FLG_NEG : 0;

    return true;
}
