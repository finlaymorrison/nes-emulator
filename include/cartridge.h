#include "mem.h"

#include <string>

#include <cstdint>

class Cartridge
{
public:
    using PRGROM = Mem<1<<14>;
    using CHRROM = Mem<1<<13>;
private:
    PRGROM prg_rom;
    CHRROM chr_rom;
public:
    Cartridge(const std::string &rom_path);

    PRGROM *prg_ref();
    CHRROM *chr_ref();
};