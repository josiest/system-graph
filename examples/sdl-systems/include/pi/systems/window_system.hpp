#pragma once
#include <iterator>
#include <algorithm>

#include <array>
#include <string_view>

#include <memory>
#include <cstdio>

#include <SDL2/SDL_video.h>
#include <entt/core/fwd.hpp>

#include "pi/systems/system_graph.hpp"
#include "pi/systems/init_system.hpp"
#include "pi/systems/sdl_deleter.hpp"

inline namespace pi {

class window_system {
public:
    using unique_window = std::unique_ptr<SDL_Window, sdl_deleter>;

    template<std::output_iterator<entt::id_type> TypeOutput>
    inline static TypeOutput dependencies(TypeOutput into_dependencies)
    {
        namespace ranges = std::ranges;
        return ranges::copy(
                std::array{ entt::type_hash<init_system>::value() },
                into_dependencies).out;
    }
    inline static window_system* load(system_graph& systems)
    {
        if (not systems.load<init_system>()) { return nullptr; }

        // config params
        constexpr std::string_view name = "A Simple Window";
        constexpr SDL_Point position{
            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED
        };
        constexpr SDL_Point size{ 640, 480 };
        constexpr auto flags = 0u;

        auto* window = SDL_CreateWindow(name.data(),
                                        position.x, position.y,
                                        size.x, size.y, flags);
        if (not window) {
            std::printf("Failed to create a window: %s\n", SDL_GetError());
            return nullptr;
        }
        return &systems.emplace<window_system>(unique_window{ window });
    }
    SDL_Window* window() { return window_handle.get(); }

    unique_window window_handle;
};
}
