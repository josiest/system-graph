#pragma once
#include <functional>
#include <algorithm>
#include <ranges>

#include <unordered_set>
#include <unordered_map>
#include <utility>
#include <vector>
#include <deque>

inline namespace pi {
#ifndef hashable
template<typename T>
concept hashable = requires(T val) {
    { std::hash<T>{}(val) } -> std::unsigned_integral;
};
#endif

inline namespace graphs {
template<hashable Vertex>
using vertex_set = std::unordered_set<Vertex>;

template<hashable Vertex>
struct directed_edge_set { vertex_set<Vertex> incoming, outgoing; };

template<hashable Vertex>
using directed_adjacency_map =
    std::unordered_map<Vertex, directed_edge_set<Vertex>>;

enum class direction{ forward, reverse };
namespace internal {

template<direction Direction, hashable Vertex>
auto& parents_of(directed_edge_set<Vertex>& edges)
{
    if constexpr (Direction == direction::forward) {
        return edges.incoming;
    }
    else {
        return edges.outgoing;
    }
}

template<direction Direction, hashable Vertex>
const auto& parents_of(const directed_edge_set<Vertex>& edges)
{
    if constexpr (Direction == direction::forward) {
        return edges.incoming;
    }
    else {
        return edges.outgoing;
    }
}

template<direction Direction, hashable Vertex>
auto& children_of(directed_edge_set<Vertex>& edges)
{
    if constexpr (Direction == direction::forward) {
        return edges.outgoing;
    }
    else {
        return edges.incoming;
    }
}

template<direction Direction, hashable Vertex>
const auto& children_of(const directed_edge_set<Vertex>& edges)
{
    if constexpr (Direction == direction::forward) {
        return edges.outgoing;
    }
    else {
        return edges.incoming;
    }
}

template<hashable Vertex>
auto into_vertices(vertex_set<Vertex>& verts)
{
    return std::inserter(verts, verts.begin());
}

template<hashable Vertex, std::ranges::input_range SourceRange>
requires std::same_as<std::ranges::range_value_t<SourceRange>, Vertex>

void add_incoming_edges(directed_adjacency_map<Vertex>& g,
                        SourceRange && sources, Vertex to)
{
    namespace ranges = std::ranges;
    if (auto search = g.find(to); search != g.end()) {
        auto& edges = search->second;
        ranges::copy(sources, into_vertices(edges.incoming));
    }
    else {
        directed_edge_set<Vertex> edges;
        ranges::copy(sources, into_vertices(edges.incoming));
        g.emplace(to, edges);
    }
}
template<hashable Vertex, std::ranges::input_range DestRange>
requires std::same_as<std::ranges::range_value_t<DestRange>, Vertex>
void add_outgoing_edges(directed_adjacency_map<Vertex>& g, Vertex from,
                        DestRange && destinations)
{
    namespace ranges = std::ranges;
    if (auto search = g.find(from); search != g.end()) {
        auto& edges = search->second;
        ranges::copy(destinations, into_vertices(edges.outgoing));
    }
    else {
        directed_edge_set<Vertex> edges;
        ranges::copy(destinations, into_vertices(edges.outgoing));
        g.emplace(from, edges);
    }
}
}

template<hashable Vertex>
void add_edge(directed_adjacency_map<Vertex>& g, Vertex from, Vertex to)
{
    namespace views = std::views;
    internal::add_incoming_edges(g, views::single(from), to);
    internal::add_outgoing_edges(g, from, views::single(to));
}
template<hashable Vertex, std::ranges::forward_range SourceRange>
requires std::same_as<std::ranges::range_value_t<SourceRange>, Vertex>
void add_edges_from(directed_adjacency_map<Vertex>& g,
                    const SourceRange& sources, Vertex to)
{
    namespace views = std::views;
    internal::add_incoming_edges(g, sources, to);
    for (const Vertex from : sources) {
        internal::add_outgoing_edges(g, from, views::single(to));
    }
}
template<hashable Vertex, std::ranges::forward_range DestRange>
requires std::same_as<std::ranges::range_value_t<DestRange>, Vertex>
void add_edges_to(directed_adjacency_map<Vertex>& g,
                  Vertex from, const DestRange& destinations)
{
    namespace views = std::views;
    internal::add_outgoing_edges(g, from, destinations);
    for (const Vertex to : destinations) {
        internal::add_incoming_edges(g, views::single(from), to);
    }
}

// BFS a cut of a graph
template<direction Direction, hashable Vertex,
         std::invocable<Vertex> Visitor, std::invocable<Vertex> Predicate>
requires std::same_as<std::invoke_result_t<Predicate, Vertex>, bool>

void bfs_cut(const directed_adjacency_map<Vertex>& g, Vertex root,
             Visitor visit, Predicate should_cut)
{
    namespace ranges = std::ranges; namespace views = std::views;
    using namespace internal;

    std::deque next = { root };
    vertex_set seen = { root };
    auto not_seen = [&](Vertex vertex) {
        return g.contains(vertex) and not seen.contains(vertex);
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

        // bfs iteration: visit branches from current directed_edge_set
        // only add edges that are haven't been explored
        const auto& outgoing = children_of<Direction>(search->second);
        ranges::copy_if(outgoing, std::back_inserter(next), not_seen);
        ranges::copy(outgoing, std::inserter(seen, seen.begin()));
    }
}

namespace internal {

template<hashable Vertex>
auto is_root_of(const directed_adjacency_map<Vertex>& g)
{
    return [&g](const directed_adjacency_map<Vertex>::value_type& elem) {
        return elem.second.incoming.empty();
    };
}
template<hashable Vertex>
auto is_leaf_of(const directed_adjacency_map<Vertex>& g)
{
    return [&g](const directed_adjacency_map<Vertex>::value_type& elem) {
        return elem.second.outgoing.empty();
    };
}

template<direction Direction, hashable Vertex>
auto cut_if_unvisited(const directed_adjacency_map<Vertex>& g,
                      const vertex_set<Vertex>& visited)
{
    namespace ranges = std::ranges;
    using namespace internal;

    return [&](const auto vertex) {
        const auto search = g.find(vertex);
        if (g.end() == search) { return false; }

        const auto& incoming = parents_of<Direction>(search->second);
        return not ranges::includes(visited, incoming);
    };
}

template<hashable Vertex>
auto cut_if_unvisited_parents(const directed_adjacency_map<Vertex>& g,
                              const vertex_set<Vertex>& visited)
{
    return cut_if_unvisited<direction::forward>(g, visited);
}
template<hashable Vertex>
auto cut_if_unvisited_children(const directed_adjacency_map<Vertex>& g,
                               const vertex_set<Vertex>& visited)
{
    return cut_if_unvisited<direction::reverse>(g, visited);
}

template<hashable Vertex, std::invocable<Vertex> Visitor>
auto visit_with_set(Visitor visit, vertex_set<Vertex>& visited)
{
    return [&visited, visit](Vertex vertex) {
        std::invoke(visit, vertex);
        visited.emplace(vertex);
    };
}
}

template<hashable Vertex, std::invocable<Vertex> Visitor>
void for_each(const directed_adjacency_map<Vertex>& g, Visitor visit)
{
    namespace ranges = std::ranges; namespace views = std::views;
    using namespace internal;

    std::vector<Vertex> roots;
    ranges::transform(g | views::filter(is_root_of(g)),
                      std::back_inserter(roots),
                      &directed_adjacency_map<Vertex>::value_type::first);

    vertex_set<Vertex> visited;
    constexpr auto forward = direction::forward;
    for (const auto root : roots) {
        bfs_cut<forward>(g, root, visit_with_set(visit, visited),
                                  cut_if_unvisited_parents(g, visited));
    };
}

template<hashable Vertex, std::invocable<Vertex> Visitor>
void rfor_each(const directed_adjacency_map<Vertex>& g, Visitor visit)
{
    namespace ranges = std::ranges; namespace views = std::views;
    using namespace internal;
    using adjacency_mapping = directed_adjacency_map<Vertex>::value_type;

    std::vector<Vertex> leafs;
    ranges::transform(g | views::filter(is_leaf_of(g)),
                      std::back_inserter(leafs), &adjacency_mapping::first);

    vertex_set<Vertex> visited;
    constexpr auto reverse = direction::reverse;
    for (const auto leaf : leafs) {
        bfs_cut<reverse>(g, leaf, visit_with_set(visit, visited),
                                  cut_if_unvisited_children(g, visited));
    };
}
}
}
