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
    
    uint8_t prg_rom_kb = header[4]*16;
    uint8_t chr_rom_kb = header[5]*8;
    std::cout << "Reading NES file - PRG:" << (int)prg_rom_kb
        << "KB, CHR:" << (int)chr_rom_kb << "KB" << std::endl;

    prg_rom.resize(prg_rom_kb*1024);
    file.read(reinterpret_cast<char*>(prg_rom.data()), prg_rom_kb*1024);
    if (file.gcount() != static_cast<std::streamsize>(prg_rom_kb*1024))
    {
        throw std::runtime_error("Failed to read PRG ROM data.");
    }

    chr_rom.resize(chr_rom_kb*1024);
    file.read(reinterpret_cast<char*>(chr_rom.data()), chr_rom_kb*1024);
    if (file.gcount() != static_cast<std::streamsize>(chr_rom_kb*1024))
    {
        throw std::runtime_error("Failed to read CHR ROM data.");
    }
}