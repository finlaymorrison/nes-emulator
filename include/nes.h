#include "cartridge.h"
#include "cpu.h"
#include "bus.h"
#include "mem.h"

#include <string>

class NES
{
private:
    using CPUMem = Mem<1<<11>;

    Bus cpu_bus;
    Bus ppu_bus;
    
    CPU cpu;
    CPUMem cpu_mem;
    Cartridge cartridge;
public:
    NES(const std::string &rom_path);

    void run();
};