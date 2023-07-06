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

enum class direction{ forward, reverse };
namespace internal {

template<direction Direction>
auto& parents_of(edge_set& edges)
{
    if constexpr (Direction == direction::forward) {
        return edges.incoming;
    }
    else {
        return edges.outgoing;
    }
}

template<direction Direction>
const auto& parents_of(const edge_set& edges)
{
    if constexpr (Direction == direction::forward) {
        return edges.incoming;
    }
    else {
        return edges.outgoing;
    }
}

template<direction Direction>
auto& children_of(edge_set& edges)
{
    if constexpr (Direction == direction::forward) {
        return edges.outgoing;
    }
    else {
        return edges.incoming;
    }
}

template<direction Direction>
const auto& children_of(const edge_set& edges)
{
    if constexpr (Direction == direction::forward) {
        return edges.outgoing;
    }
    else {
        return edges.incoming;
    }
}

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

// BFS a cut of a graph
template<direction Direction, std::invocable<entt::id_type> Visitor,
                              std::invocable<entt::id_type> Predicate>

requires std::same_as<std::invoke_result_t<Predicate, entt::id_type>, bool>
void bfs_cut(const graph& g, entt::id_type root,
             Visitor visit, Predicate should_cut)
{
    namespace ranges = std::ranges; namespace views = std::views;
    using namespace internal;

    std::deque next = { root };
    id_set seen = { root };
    auto not_seen = [&](entt::id_type id) {
        return g.contains(id) and not seen.contains(id);
    };

    while (not next.empty()) {
        auto from = next.front();
        next.pop_front();

        if (std::invoke(should_cut, from)) {
            continue;
        }
        std::invoke(visit, from);

        const auto search = g.find(from);
        if (g.end() == search) { continue; }

        // bfs iteration: visit branches from current edge_set
        // only add edges that are haven't been explored
        const id_set& outgoing = children_of<Direction>(search->second);
        ranges::copy_if(outgoing, std::back_inserter(next), not_seen);
        ranges::copy(outgoing, std::inserter(seen, seen.begin()));
    }
}

namespace internal {
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

template<direction Direction>
auto cut_if_unvisited(const graph& g, const id_set& visited)
{
    namespace ranges = std::ranges;
    using namespace internal;

    return [&](const auto id) {
        const auto search = g.find(id);
        if (g.end() == search) { return false; }

        const id_set& incoming = parents_of<Direction>(search->second);
        return not ranges::includes(visited, incoming);
    };
}

auto cut_if_unvisited_parents(const graph& g, const id_set& visited)
{
    return cut_if_unvisited<direction::forward>(g, visited);
}
auto cut_if_unvisited_children(const graph& g, const id_set& visited)
{
    return cut_if_unvisited<direction::reverse>(g, visited);
}

template<std::invocable<entt::id_type> Visitor>
auto visit_with_set(Visitor visit,
                    std::unordered_set<entt::id_type>& visited)
{
    return [&visited, visit](entt::id_type id) {
        std::invoke(visit, id);
        visited.emplace(id);
    };
}
}

template<std::invocable<entt::id_type> Visitor>
void for_each(const graph& g, Visitor visit)
{
    namespace ranges = std::ranges; namespace views = std::views;
    using namespace internal;

    std::vector<entt::id_type> roots;
    ranges::transform(g | views::filter(is_root_of(g)),
                      std::back_inserter(roots), &graph::value_type::first);

    id_set visited;
    constexpr auto forward = direction::forward;
    for (const auto root : roots) {
        bfs_cut<forward>(g, root, visit_with_set(visit, visited),
                                  cut_if_unvisited_parents(g, visited));
    };
}

template<std::invocable<entt::id_type> Visitor>
void rfor_each(const graph& g, Visitor visit)
{
    namespace ranges = std::ranges; namespace views = std::views;
    using namespace internal;

    std::vector<entt::id_type> leafs;
    ranges::transform(g | views::filter(is_leaf_of(g)),
                      std::back_inserter(leafs), &graph::value_type::first);

    id_set visited;
    constexpr auto reverse = direction::reverse;
    for (const auto leaf : leafs) {
        bfs_cut<reverse>(g, leaf, visit_with_set(visit, visited),
                                  cut_if_unvisited_children(g, visited));
    };
}
}
}
