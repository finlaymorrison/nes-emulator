#include <vector>

#include <string>

#include <cstdint>

class Cartridge
{
private:
    std::vector<uint8_t> prg_rom;
    std::vector<uint8_t> chr_rom;
public:
    Cartridge(const std::string &rom_path);
};