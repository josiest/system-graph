#pragma once

#include <utility> // std::exchange
#include <string_view>
#include <cstdio>

#include <SDL2/SDL.h>
#include "pi/systems/system_graph.hpp"

inline namespace pi {
class init_system : public ISystem {
public:
#pragma region Rule of Five
    init_system(const init_system&) = delete;
    init_system& operator=(const init_system&) = delete;

    inline init_system(init_system && tmp)
        : should_quit{ std::exchange(tmp.should_quit, false) }
    {
    }
    inline init_system& operator=(init_system && tmp)
    {
        should_quit = std::exchange(tmp.should_quit, false);
        return *this;
    }
    inline ~init_system() { destroy(); }
#pragma endregion

#pragma region ISystem
    inline static init_system* load(system_graph& systems)
    {
        if (SDL_Init(SDL_INIT_VIDEO) != 0) {
            std::printf("Failed to initialize SDL: %s\n", SDL_GetError());
            return nullptr;
        }
        return &systems.emplace<init_system>(init_system{});
    }
    inline virtual void destroy() override
    {
        if (should_quit) {
            // TODO: log verbose
            // std::printf("quit sdl\n");
            SDL_Quit(); should_quit = false;
        }
    }
    inline virtual constexpr std::string_view name() const override
    {
        return "SDL Init System";
    }
#pragma endregion
private:
    init_system() = default;
    bool should_quit = true;
};
}
