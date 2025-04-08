#include "ppu.h"
#include "bus.h"

PPU::PPU() :
    registers{}
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