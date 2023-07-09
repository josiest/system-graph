#include <cstddef>
#include <cstdio>

#include <SDL2/SDL_render.h>

#include "pi/systems/system_graph.hpp"
#include "pi/systems/renderer_system.hpp"
#include "fibonacci_spiral.hpp"

int main()
{
    pi::system_graph systems;
    auto* renderer_system = systems.load<pi::renderer_system>();

    if (not renderer_system) {
        std::printf("Fatal error: unable to load fundamental systems\n");
        return EXIT_FAILURE;
    }
    SDL_Renderer* renderer = renderer_system->renderer_handle.get();

    // design parameters
    constexpr SDL_Color blue{ 48, 118, 217, 255 };
    constexpr SDL_Color red{ 219, 0, 66, 255 };
    constexpr std::uint32_t num_frames = 9u;

    fibonacci_spiral(blue, red, num_frames)
        .draw_rects_to(renderer);

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
    return EXIT_SUCCESS;
}
