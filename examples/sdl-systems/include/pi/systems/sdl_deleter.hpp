#pragma once
#include <cstdio>
#include <SDL2/SDL_video.h>
#include <SDL2/SDL_render.h>

inline namespace pi {
struct sdl_deleter {
    inline void operator()(SDL_Window* window)
    {
        if (window) { 
            // TODO: log verbose
            // std::printf("destroy window\n");
            SDL_DestroyWindow(window);
        }
    }
    inline void operator()(SDL_Renderer* renderer)
    {
        if (renderer) {
            // TODO: log verbose
            // std::printf("destroy renderer\n");
            SDL_DestroyRenderer(renderer);
        }
    }
};
}
