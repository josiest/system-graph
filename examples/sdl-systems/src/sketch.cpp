#include <concepts>
#include <numbers>

#include <cstdint>
#include <cstddef>
#include <cmath>

#include <cstdio>

#include <SDL2/SDL.h>
#include "pi/systems/system_graph.hpp"
#include "pi/systems/renderer_system.hpp"

// truncate-lerp between two integers
template<std::integral Integer, std::floating_point Real>
constexpr Integer lerp(Integer a, Integer b, Real t)
{
    return static_cast<Integer>(std::lerp(static_cast<Real>(a),
                                          static_cast<Real>(b), t));
}
// lerp between two colors
template<std::floating_point Real>
constexpr SDL_Color lerp(const SDL_Color& a, const SDL_Color& b, Real t)
{
    return SDL_Color{ lerp(a.r, b.r, t), lerp(a.g, b.g, t),
                      lerp(a.b, b.b, t), lerp(a.a, b.a, t) };
}
// create a fibonacci spiral with a gradient
template<std::unsigned_integral Integer>
struct colored_rect_sequence {
    auto operator()(Integer k)
    {
        using namespace std::numbers;
        const float t = static_cast<float>(k)/N;
        const SDL_Rect rect = guide;
        const SDL_Point frame{ static_cast<int>(rect.w/phi_v<float>),
                               static_cast<int>(rect.h/phi_v<float>) };

        switch (k % 4) {
        case 0:
            guide.x += frame.x; guide.w -= frame.x;
            break;
        case 1:
            guide.y += frame.y; guide.h -= frame.y;
            break;
        case 2:
            guide.w = frame.x;
            break;
        case 3:
            guide.h = frame.y;
            break;
        default: break;
        }
        return std::make_pair(rect, lerp(first, last, t));
    }
    SDL_Rect guide;
    SDL_Color first, last;
    Integer N;
};

auto draw_rect_to(SDL_Renderer* renderer)
{
    return [=](const std::pair<SDL_Rect, SDL_Color>& colored_rect) {
        const auto& [rect, color] = colored_rect;
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
        SDL_RenderFillRect(renderer, &rect);
    };
}

int main()
{
    namespace ranges = std::ranges;
    namespace views = std::views;

    pi::system_graph systems;
    auto* renderer_system = systems.load<pi::renderer_system>();

    if (not renderer_system) {
        std::printf("Fatal error: unable to load fundamental systems\n");
        return EXIT_FAILURE;
    }
    SDL_Renderer* renderer = renderer_system->renderer_handle.get();

    constexpr SDL_Color blue{ 48, 118, 217, 255 };
    constexpr SDL_Color red{ 219, 0, 66, 255 };

    // design parameters
    constexpr SDL_Color first = blue;
    constexpr SDL_Color last = red;
    constexpr std::uint32_t N = 9u;

    // the pattern will be draw to the entire screen
    SDL_Rect render_frame{ 0, 0, 0, 0 };
    SDL_GetRendererOutputSize(renderer, &render_frame.w, &render_frame.h);

    // sequence that accepts index and giv pair of rect and color
    auto make_rect = colored_rect_sequence(render_frame, first, last, N);

    // render the entire frame with the last color
    // since the last rect won't be drawn
    SDL_SetRenderDrawColor(renderer, last.r, last.g, last.b, last.a);
    SDL_RenderClear(renderer);

    // render each subframe with a different color
    ranges::for_each(views::iota(0u, N) | views::transform(make_rect),
                     draw_rect_to(renderer));
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
