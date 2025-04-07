#include "singlesteptests.h"
#include "nes.h"

#include <string>
#include <iostream>

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        std::cerr << "Must provide 1 argument" << std::endl;
    }
    if (std::string(argv[1]) == "singlesteptests")
    {
        const std::string single_step_dir = "external/65x02/nes6502/v1";
        run_tests(single_step_dir);
    }
    else if (std::string(argv[1]) == "rom")
    {
        const std::string rom_path = "roms/Donkey Kong (USA) (Rev 1) (e-Reader Edition).nes";
        NES nes(rom_path);
    }
}
