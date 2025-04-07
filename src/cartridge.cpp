#include "cartridge.h"

#include <stdexcept>
#include <fstream>
#include <iostream>

Cartridge::Cartridge(const std::string &rom_path)
{
    std::ifstream file(rom_path, std::ios::binary);
    if (!file.is_open())
    {
        throw std::runtime_error("Failed to open ROM file.");
    }
    uint8_t header[16];
    file.read(reinterpret_cast<char*>(header), 16);
    if (file.gcount() != 16
        || header[0] != 'N' || header[1] != 'E' || header[2] != 'S'
        || header[3] != 0x1A)
    {
        throw std::runtime_error("Invalid iNES file.");
    }
    
    int prg_rom_kb = header[4]*16*1024;
    int chr_rom_kb = header[5]*8*1024;
    uint8_t mapper = header[7]&0xF0 | (header[6]>>4);
    std::cout << "Reading NES file [MAP:" << (int)mapper
        << "] - PRG:" << prg_rom_kb/1024 << "KB, CHR:"
        << chr_rom_kb/1024 << "KB" << std::endl;

    prg_rom.load_binary_region(rom_path, 16, prg_rom_kb);
    chr_rom.load_binary_region(rom_path, 16 + prg_rom_kb, chr_rom_kb);
}

Cartridge::PRGROM *Cartridge::prg_ref()
{
    return &prg_rom;
}
Cartridge::CHRROM *Cartridge::chr_ref()
{
    return &chr_rom;
}