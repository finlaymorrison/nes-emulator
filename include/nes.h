#include "cartridge.h"

#include <string>

class NES
{
private:
    Cartridge cartridge;
public:
    NES(const std::string &rom_path);
};