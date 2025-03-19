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

    int op_grp = ins_stack[0]&0b11;

    bool complete;
    switch ((ins_stack[0]&0b00011100)>>2)
    {
    case 0b000:
        // INX, IM, IM
        if (op_grp == 0)
        {
            complete = MODE_INX();
        }
        else
        {
            complete = MODE_IM();
        }
    break;
    case 0b001:
        // ZP, ZP, ZP
        complete = MODE_ZP();
    break;
    case 0b010:
        // IM, IMP, _
        if (op_grp == 0)
        {
            complete = MODE_IM();
        }
        else
        {
            complete = MODE_IMP(); 
        }
    break;
    case 0b011:
        // AB, AB, AB/IN
        if (ins_stack[0] == 0b01001100)
        {
            complete = MODE_IN();
        }
        else
        {
            complete = MODE_AB();
        }
    break;
    case 0b100:
        // INY, _, _
        complete = MODE_INY();
    break;
    case 0b101:
        // ZPX, ZPX, ZPX
        complete = MODE_ZPX();
    break;
    case 0b110:
        // ABY, _, _
        complete = MODE_ABY();
    break;
    case 0b111:
        // ABX, ABX, ABX
        complete = MODE_ABX();
    break;
    }

    if (complete)
    {
        ++ins_step;
        return;
    }

    ins_step = 0;
    ins_stack.clear();
}
