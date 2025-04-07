#include "nes.h"

NES::NES(const std::string &rom_path) :
    cpu_bus(), ppu_bus(),
    cpu(), cpu_mem(),
    ppu(), palette_mem(),
    cartridge(rom_path)
{
    cpu_bus.map_device(0x0000, 0x1FFF, &cpu_mem);
    cpu_bus.map_device(0x2000, 0x3FFF, ppu.reg_ref());
    // cpu_bus.map_device(0x4000, 0x4017, APU + IO Registers);
    cpu_bus.map_device(0x8000, 0xFFFF, cartridge.prg_ref());

    cpu.attach_bus(&cpu_bus);

    ppu_bus.map_device(0x0000, 0x1FFF, cartridge.chr_ref());
    ppu_bus.map_device(0x2000, 0x2FFF, cartridge.vram_ref());
    ppu_bus.map_device(0x3F00, 0x3FFF, &palette_mem);

    ppu.attach_bus(&ppu_bus);
}

void NES::run()
{

}