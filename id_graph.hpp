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
struct edge_set { id_set incoming, outgoing; };
using graph = std::unordered_map<entt::id_type, edge_set>;

namespace internal {
auto into_edges(id_set& ids)
{
    return std::inserter(ids, ids.begin());
}

template<std::ranges::input_range SourceRange>
requires std::same_as<std::ranges::range_value_t<SourceRange>, entt::id_type>
void add_incoming_edges(graph& g, SourceRange && sources, entt::id_type to)
{
    namespace ranges = std::ranges;
    if (auto search = g.find(to); search != g.end()) {
        auto& edges = search->second;
        ranges::copy(sources, into_edges(edges.incoming));
    }
    else {
        edge_set edges;
        ranges::copy(sources, into_edges(edges.incoming));
        g.emplace(to, edges);
    }
}
template<std::ranges::input_range DestRange>
requires std::same_as<std::ranges::range_value_t<DestRange>, entt::id_type>
void add_outgoing_edges(graph& g, entt::id_type from,
                        DestRange && destinations)
{
    namespace ranges = std::ranges;
    if (auto search = g.find(from); search != g.end()) {
        auto& edges = search->second;
        ranges::copy(destinations, into_edges(edges.outgoing));
    }
    else {
        edge_set edges;
        ranges::copy(destinations, into_edges(edges.outgoing));
        g.emplace(from, edges);
    }
}
}

void add_edge(graph& g, entt::id_type from, entt::id_type to)
{
    namespace views = std::views;
    internal::add_incoming_edges(g, views::single(from), to);
    internal::add_outgoing_edges(g, from, views::single(to));
}
template<std::ranges::forward_range SourceRange>
requires std::same_as<std::ranges::range_value_t<SourceRange>, entt::id_type>
void add_edges_from(graph& g, const SourceRange& sources, entt::id_type to)
{
    namespace views = std::views;
    internal::add_incoming_edges(g, sources, to);
    for (const entt::id_type from : sources) {
        internal::add_outgoing_edges(g, from, views::single(to));
    }
}
template<std::ranges::forward_range DestRange>
requires std::same_as<std::ranges::range_value_t<DestRange>, entt::id_type>
void add_edges_to(graph& g, entt::id_type from, const DestRange& destinations)
{
    namespace views = std::views;
    internal::add_outgoing_edges(g, from, destinations);
    for (const entt::id_type to : destinations) {
        internal::add_incoming_edges(g, views::single(from), to);
    }
}

namespace internal {
enum class direction{ forward, reverse };

// BFS a cut of a graph
// such that no branches with unvisited incoming are explored
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

        const id_set* incoming;
        const id_set* outgoing;
        auto search = g.find(from);
        if constexpr (Direction == direction::forward) {
            incoming = &search->second.incoming;
            outgoing = &search->second.outgoing;
        }
        else {
            incoming = &search->second.outgoing;
            outgoing = &search->second.incoming;
        }

        // don't visit branches with unvisited dependencies
        if (not ranges::includes(visited, *incoming)) {
            continue;
        }
        std::invoke(visit, from);
        visited.insert(from);

        // bfs iteration: visit branches from current edge_set
        // only add edges that are haven't been explored
        ranges::copy_if(*outgoing, std::back_inserter(next), not_seen);
        ranges::copy(*outgoing, std::inserter(seen, seen.begin()));
    }
}

auto is_root_of(const graph& g)
{
    return [&g](const graph::value_type& elem) {
        return elem.second.incoming.empty();
    };
}
auto is_leaf_of(const graph& g)
{
    return [&g](const graph::value_type& elem) {
        return elem.second.outgoing.empty();
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
