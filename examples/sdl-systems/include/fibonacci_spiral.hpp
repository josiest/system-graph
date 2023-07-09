#pragma once
#include <concepts>
#include <algorithm>

#include <functional>
#include <numbers>
#include <utility>

#include <cstdint>
#include <cmath>

#include <SDL2/SDL_render.h>

/** Truncate-lerp between two integers */
template<std::integral Integer, std::floating_point Real>
constexpr Integer lerp(Integer a, Integer b, Real t)
{
    return static_cast<Integer>(std::lerp(static_cast<Real>(a),
                                          static_cast<Real>(b), t));
}
/** Lerp between two colors */
template<std::floating_point Real>
constexpr SDL_Color lerp(const SDL_Color& a, const SDL_Color& b, Real t)
{
    return SDL_Color{ lerp(a.r, b.r, t), lerp(a.g, b.g, t),
                      lerp(a.b, b.b, t), lerp(a.a, b.a, t) };
}
/** Transform a guide rect into the next subframe of the fibonacci spiral
 *
 * \param guide changed into the next fibonacci spiral frame
 * \param k     used to determine which edge to transform
 *
 * \return the current value of the guide rect unchanged
 */
constexpr SDL_Rect next_subframe(SDL_Rect& guide, std::uint32_t k)
{
    using namespace std::numbers;
    const SDL_Rect rect = guide;
    const SDL_Point frame{ static_cast<int>(guide.w/phi_v<float>),
                           static_cast<int>(guide.h/phi_v<float>) };
    switch (k % 4) {
    case 0:
        // slide the left edge inward by the golden ratio
        guide.x += frame.x; guide.w -= frame.x;
        break;
    case 1:
        // slide the top edge down by the golden ratio
        guide.y += frame.y; guide.h -= frame.y;
        break;
    case 2:
        // slide the right edge inward by the golden ratio
        guide.w -= frame.x;
        break;
    case 3:
        // slide the bottom edge upward by the golden ratio
        guide.h -= frame.y;
        break;
    default: break;
    }
    return rect;
}
/** A rect and the color it should be drawn with */
using colored_rect = std::pair<SDL_Rect, SDL_Color>;

/** Draw a colored rect to a renderer */
inline auto draw_rect(SDL_Renderer* renderer, const colored_rect& frame)
{
    const auto& [rect, color] = frame;
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(renderer, &rect);
}

/** System that draws fibonacci spirals to a renderer */
struct fibonacci_spiral {
    /** Generate the fibonacci spiral sequence
     *
     * \param guide The frame to generate te spiral in
     * \return a function that generates the fibonacci spiral sequence
     *
     * As a side effect, the guide will be changed with every call
     * of the generator function
     */
    inline auto sequence(SDL_Rect& guide) const
    {
        return [this, &guide](std::uint32_t k) {
            const float t = static_cast<float>(k)/num_frames;
            return std::make_pair(next_subframe(guide, k),
                                  lerp(initial_color, final_color, t));
        };
    }
    /** Draw the fibonacci spiral to the renderer */
    inline void draw_rects_to(SDL_Renderer* renderer) const
    {
        namespace ranges = std::ranges;
        namespace views = std::views;

        // the pattern will be draw to the entire screen
        SDL_Rect render_frame{ 0, 0, 0, 0 };
        SDL_GetRendererOutputSize(renderer, &render_frame.w,
                                            &render_frame.h);

        // render each subframe with a different color
        ranges::for_each(views::iota(0u, num_frames)
                            | views::transform(sequence(render_frame)),
                         std::bind_front(&draw_rect, renderer));
    }
    SDL_Color initial_color, final_color;
    std::uint32_t num_frames;
};
