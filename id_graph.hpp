#pragma once
#include <functional>
#include <algorithm>
#include <ranges>

#include <unordered_set>
#include <unordered_map>
#include <utility>
#include <vector>
#include <deque>

#include <entt/core/fwd.hpp>

namespace pi {
using id_set = std::unordered_set<entt::id_type>;

namespace graphs {
struct node { id_set parents, children; };
using graph = std::unordered_map<entt::id_type, node>;

namespace internal {
enum class direction{ forward, reverse };

// BFS a cut of a graph
// such that no branches with unvisited parents are explored
template<direction Direction, std::invocable<entt::id_type> Visitor>
void bfs(const graph& g, entt::id_type root, id_set& visited, Visitor visit)
{
    namespace ranges = std::ranges; namespace views = std::views;

    std::deque next = { root };
    id_set seen = { root };
    auto not_seen = [&](entt::id_type id) {
        return g.contains(id) and not seen.contains(id);
    };

    while (not next.empty()) {
        auto from = next.front();
        next.pop_front();

        const id_set* parents;
        const id_set* children;
        auto search = g.find(from);
        if constexpr (Direction == direction::forward) {
            parents = &search->second.parents;
            children = &search->second.children;
        }
        else {
            parents = &search->second.children;
            children = &search->second.parents;
        }

        // don't visit branches with unvisited dependencies
        if (not ranges::includes(visited, *parents)) {
            continue;
        }
        std::invoke(visit, from);
        visited.insert(from);

        // bfs iteration: visit branches from current node
        // only add nodes that are haven't been explored
        ranges::copy_if(*children, std::back_inserter(next), not_seen);
        ranges::copy(*children, std::inserter(seen, seen.begin()));
    }
}

auto is_root_of(const graph& g)
{
    return [&g](const graph::value_type& elem) {
        return elem.second.parents.empty();
    };
}
auto is_leaf_of(const graph& g)
{
    return [&g](const graph::value_type& elem) {
        return elem.second.children.empty();
    };
}
}

template<std::invocable<entt::id_type> Visitor>
void for_each(const graph& g, Visitor visit)
{
    namespace ranges = std::ranges; namespace views = std::views;
    std::vector<entt::id_type> roots;
    ranges::transform(g | views::filter(internal::is_root_of(g)),
                      std::back_inserter(roots), &graph::value_type::first);

    id_set visited;
    constexpr auto forward = internal::direction::forward;
    for (const auto root : roots) {
        internal::bfs<forward>(g, root, visited, visit);
    };
}

template<std::invocable<entt::id_type> Visitor>
void rfor_each(const graph& g, Visitor visit)
{
    namespace ranges = std::ranges; namespace views = std::views;
    std::vector<entt::id_type> leafs;
    ranges::transform(g | views::filter(internal::is_leaf_of(g)),
                      std::back_inserter(leafs), &graph::value_type::first);

    id_set visited;
    constexpr auto reverse = internal::direction::reverse;
    for (const auto leaf : leafs) {
        internal::bfs<reverse>(g, leaf, visited, visit);
    };
}
}
}
