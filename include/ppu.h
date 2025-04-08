#pragma once

#include "mem.h"

class Bus;

constexpr int RESOLUTION_X = 256;
constexpr int RESOLUTION_Y = 240;

class PPU
{
public:
    using Display = std::array<std::array<uint8_t, RESOLUTION_Y>, RESOLUTION_X>;
private:
    using Registers = Mem<8>;

    Bus *bus;
    Registers registers;
    Display display;
public:
    PPU();
    Registers *reg_ref();

    void attach_bus(Bus *new_bus);
    void clock_cycle();
    Display get_display();
    bool frame_complete();
};