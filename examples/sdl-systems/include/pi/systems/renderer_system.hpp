#pragma once
#include <iterator>
#include <algorithm>

#include <array>
#include <string_view>

#include <memory>
#include <cstdio>

#include <SDL2/SDL_render.h>
#include <entt/core/fwd.hpp>

#include "pi/systems/system_graph.hpp"
#include "pi/systems/window_system.hpp"
#include "pi/systems/sdl_deleter.hpp"

inline namespace pi {
class renderer_system {
public:
    using unique_renderer = std::unique_ptr<SDL_Renderer, sdl_deleter>;

    template<std::output_iterator<entt::id_type> TypeOutput>
    inline static TypeOutput dependencies(TypeOutput into_types)
    {
        namespace ranges = std::ranges;
        return ranges::copy(
                std::array{ entt::type_hash<window_system>::value() },
                into_types).out;
    }
    inline static renderer_system* load(pi::system_graph& systems)
    {
        auto* window_sys = systems.load<window_system>();
        if (not window_sys) { return nullptr; }

        // config parameters
        constexpr auto flags = SDL_RENDERER_ACCELERATED;

        auto* window = window_sys->window();
        auto* renderer = SDL_CreateRenderer(window, flags, -1);

        if (not renderer) {
            std::printf("Failed to create a renderer: %s\n", SDL_GetError());
        }
        return &systems.emplace<renderer_system>(unique_renderer{ renderer });
    }
    SDL_Renderer* renderer() { return renderer_handle.get(); }

    renderer_system(unique_renderer && renderer_ptr)
        : renderer_handle{ std::move(renderer_ptr) }
    {
    }
    unique_renderer renderer_handle;
};
}
