#include "ppu.h"
#include "bus.h"

#include <iostream>
#include <iomanip>

PPU::PPU() :
    registers{}, display{}, bus{nullptr},
    v{}, w{false}, fine_x{},
    pt_l_input{}, pt_l_sr{},
    pt_h_input{}, pt_h_sr{},
    nt_id{}, attr{}
{}

PPU::Registers *PPU::reg_ref()
{
    return &registers;
}

void PPU::attach_bus(Bus *new_bus)
{
    bus = new_bus;
}

void PPU::clock_cycle()
{
    if (fine_x == 1)
    {
        // Nametable fetch
        uint16_t tile_addr = 0x2000 | (v & 0x0FFF);
        nt_id = bus->get(tile_addr);
    }

    if (fine_x == 3)
    {
        // Attribute fetch
        uint16_t attr_addr = 0x23C0 | (v & 0x0C00) | ((v >> 4) & 0x38) | ((v >> 2) & 0x07);
        attr_input = bus->get(attr_addr);
    }

    if (fine_x == 5)
    {
        // Pattern table low fetch
        pt_l_input = bus->get(nt_id<<4);
    }

    if (fine_x == 7)
    {
        // Pattern table high fetch
        pt_h_input = bus->get((nt_id<<4) | 0x8);
    }

    uint8_t pallete = pt_l_sr&0x1 | ((pt_h_sr&0x1)<<1);
    pt_l_sr >>= 1;
    pt_h_sr >>= 1;

    if (pallete != 0 && line < 240)
    {
        display[((v&0x1F)<<3) | fine_x][line] = 1;
    }

    if (fine_x == 7)
    {
        v += 1;

        // Store new pattern into upper bits of shift registers.
        pt_l_sr |= pt_l_input<<8;
        pt_h_sr |= pt_h_input<<8;
        attr_sr |= attr_input<<8;

        fine_x = 0;
    }
    else
    {
        fine_x++;
    }
}

PPU::Display PPU::get_display()
{
    return display;
}

bool PPU::frame_complete()
{
    return true;
}