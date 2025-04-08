#include "nes.h"
#include "window.h"

#include <chrono>
#include <thread>

NES::NES(Window *window, const std::string &rom_path) :
    window(window),
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
    using namespace std::chrono;

    constexpr int target_round_period = 601;

    for (int i = 0; i < 1e7; ++i)
    {
        auto round_start_time = high_resolution_clock::now();

        cpu_bus.start_cycle();
        cpu.clock_cycle();

        for (int j = 0; j < 3; ++j)
        {
            ppu_bus.start_cycle();
            ppu.clock_cycle();
            if (ppu.frame_complete() && j == 0 && i%30000 == 0)
            {
                window->draw(ppu.get_display());
            }
        }

        auto round_finish_time = high_resolution_clock::now();
        int round_period = duration_cast<nanoseconds>(
            round_finish_time - round_start_time
        ).count();
        if (target_round_period - round_period)
        {
            //std::this_thread::sleep_for(nanoseconds(target_round_period - round_period));
        }
    }
}