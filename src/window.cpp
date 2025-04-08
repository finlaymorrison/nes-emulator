#include "window.h"
#include "nes.h"

#include <iostream>
#include <array>

Window::Window(int width, int height) :
    width(width),
    height(height)
{
    window = SDL_CreateWindow("CHIP-8",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        width, height, SDL_WINDOW_SHOWN
    );
    if (window == NULL)
    {
        std::cerr << "Failed to create window : " << SDL_GetError() << std::endl;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == NULL)
    {
        std::cerr << "Failed to create renderer : " << SDL_GetError() << std::endl;
    }
}

void Window::draw(const PPU::Display &display)
{
    draw_grid(display);
    SDL_RenderPresent(renderer);
}

void Window::draw_grid(const PPU::Display &display)
{
    SDL_SetRenderDrawColor(renderer, 0,0,0,255);
    SDL_RenderClear(renderer);

    int cols = display.size();
    int rows = display[0].size();

    int square_width = width / cols;
    int square_height = height / rows;

    for (int col = 0; col < cols; ++col) {
        for (int row = 0; row < rows; ++row) {
            if (display[col][row]) {
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            } else {
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            }

            SDL_Rect rect = { col * square_width, row * square_height, square_width, square_height };
            SDL_RenderFillRect(renderer, &rect);
        }
    }
}