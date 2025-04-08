#pragma once

#include "ppu.h"

#include <SDL2/SDL.h>

class Window
{
private:
    SDL_Window *window;
    SDL_Renderer *renderer;
    int width;
    int height;
public:
    Window(int width, int height);
    void draw(const PPU::Display &display);
private:
    void draw_grid(const PPU::Display &display);
};