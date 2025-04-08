#pragma once

#include "cartridge.h"
#include "cpu.h"
#include "bus.h"
#include "mem.h"
#include "ppu.h"

#include <string>
#include <array>

class Window;

class NES
{
private:
    using CPUMem = Mem<1<<11>;
    using PaletteMem = Mem<1<<8>;

    Bus cpu_bus;
    Bus ppu_bus;

    CPU cpu;
    CPUMem cpu_mem;
    Cartridge cartridge;

    PPU ppu;
    PaletteMem palette_mem;

    Window *window;
public:
    NES(Window *window, const std::string &rom_path);

    void run();
};