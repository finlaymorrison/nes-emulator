#pragma once

#include "flagmem.h"
#include "mem.h"

#include <cstdint>

class Bus;

constexpr int RESOLUTION_X = 256;
constexpr int RESOLUTION_Y = 240;


class PPU
{
public:
    using Display = std::array<std::array<uint8_t, RESOLUTION_Y>, RESOLUTION_X>;
    using Registers = FlagMem<8>;
    using OAM = Mem<256>;
private:

    Bus *bus;
    Registers registers;
    OAM oam;
    Display display;

    uint16_t v;
    bool w;

    uint8_t fine_x;
    uint16_t line;

    uint8_t nt_id;

    uint8_t pt_l_input;
    uint16_t pt_l_sr;

    uint8_t pt_h_input;
    uint16_t pt_h_sr;


    uint8_t attr_input;
    uint16_t attr_sr;
public:
    PPU();
    Registers *reg_ref();

    void attach_bus(Bus *new_bus);
    void clock_cycle();
    Display get_display();
    bool frame_complete();
};