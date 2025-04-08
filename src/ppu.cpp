#include "ppu.h"
#include "bus.h"

PPU::PPU() :
    registers{}, display{}, bus{nullptr}
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

}

PPU::Display PPU::get_display()
{
    return display;
}

bool PPU::frame_complete()
{
    return true;
}