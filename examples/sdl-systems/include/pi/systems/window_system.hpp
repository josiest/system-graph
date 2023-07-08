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

class window_system : public ISystem {
public:
    using unique_window = std::unique_ptr<SDL_Window, sdl_deleter>;

#pragma region Rule of Five
    window_system(const window_system&) = delete;
    window_system& operator=(const window_system&) = delete;
    window_system(window_system &&) = default;
    window_system& operator=(window_system &&) = default;
    ~window_system() { destroy(); }
#pragma endregion

#pragma region ISystem
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
        auto* window = SDL_CreateWindow(
                "A simple window",
                SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                640, 480,
                SDL_WINDOW_SHOWN);

        if (not window) {
            std::printf("Failed to create a window: %s\n", SDL_GetError());
            return nullptr;
        }
        return &systems.emplace<window_system>(unique_window{ window });
    }
    inline virtual void destroy() override
    {
        window_handle.reset();
    }
    inline virtual constexpr std::string_view name() const override
    {
        return "SDL Window";
    }
#pragma endregion
    window_system(unique_window && window_ptr)
        : window_handle{ std::move(window_ptr) }
    {
    }
    unique_window window_handle;
};
}
