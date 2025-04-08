#pragma once

#include "mem.h"

class Bus;

class PPU
{
private:
    using Registers = Mem<8>;

    Bus *bus;
    Registers registers;
public:
    PPU();
    Registers *reg_ref();

    void attach_bus(Bus *new_bus);
    void clock_cycle();
};