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
class renderer_system : public ISystem {
public:
    using unique_renderer = std::unique_ptr<SDL_Renderer, sdl_deleter>;
#pragma region Rule of Five
    renderer_system(const renderer_system&) = delete;
    renderer_system& operator=(const renderer_system&) = delete;
    renderer_system(renderer_system &&) = default;
    renderer_system& operator=(renderer_system &&) = default;
    ~renderer_system() { destroy(); }
#pragma endregion

#pragma region ISystem
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
        auto* windows = systems.load<window_system>();
        if (not windows) { return nullptr; }

        auto* window = windows->window_handle.get();
        auto* renderer = SDL_CreateRenderer(
                window, SDL_RENDERER_ACCELERATED, -1);

        if (not renderer) {
            std::printf("Failed to create a renderer: %s\n", SDL_GetError());
        }
        return &systems.emplace<renderer_system>(unique_renderer{ renderer });
    }
    virtual void destroy() override
    {
        renderer_handle.reset();
    }
    virtual constexpr std::string_view name() const override
    {
        return "SDL Renderer";
    }
#pragma endregion
    renderer_system(unique_renderer && renderer_ptr)
        : renderer_handle{ std::move(renderer_ptr) }
    {
    }
    unique_renderer renderer_handle;
};
}
