#include "nes.h"

NES::NES(const std::string &rom_path) :
    cpu_bus(), ppu_bus(),
    cpu(), cpu_mem(),
    cartridge(rom_path)
{
    cpu_bus.map_device(0x0000, 0x1FFF, &cpu_mem);
    // cpu_bus.map_device(0x2000, 0x3FFF, PPU Registers);
    // cpu_bus.map_device(0x4000, 0x4017, APU + IO Registers);
    cpu_bus.map_device(0x8000, 0xFFFF, cartridge.prg_ref());

    cpu.attach_bus(&cpu_bus);
}

void NES::run()
{

}