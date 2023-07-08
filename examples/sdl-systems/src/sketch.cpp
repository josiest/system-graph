#include <cstdio>
#include <cstddef>

#include <SDL2/SDL.h>

#include "pi/systems/system_graph.hpp"
#include "pi/systems/renderer_system.hpp"

int main()
{
    pi::system_graph systems;
    auto* renderer_system = systems.load<pi::renderer_system>();

    if (not renderer_system) {
        std::printf("Fatal error: unable to load fundamental systems\n");
        return EXIT_FAILURE;
    }
    SDL_Renderer* renderer = renderer_system->renderer_handle.get();
    SDL_SetRenderDrawColor(renderer, 0xff, 0xff, 0xff, 0xff);
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);

    bool has_quit = false;
    while (not has_quit) {

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                has_quit = true;
            }
        }
    }
}
