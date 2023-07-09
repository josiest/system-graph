#pragma once
#include <concepts>
#include <iterator>

#include <unordered_map>
#include <vector>
#include <deque>

#include <memory>
#include <string_view>

#include <entt/entity/registry.hpp>
#include "pi/graphs/digraph.hpp"

#include <cstdio>

inline namespace pi {

template<typename System, std::output_iterator<entt::id_type> TypeOutput>
constexpr bool has_dependencies = requires(TypeOutput into_types)
{
    { System::dependencies(into_types) } -> std::same_as<TypeOutput>;
};

class system_graph;
template<typename System, typename... Args>
constexpr bool can_load_with =
requires(system_graph& systems, Args&&... args)
{
    { System::load(systems, std::forward<Args>(args)...) }
        -> std::same_as<System*>;
};

class system_graph {
#pragma region Rule of Five
public:
    system_graph(const system_graph&) = delete;
    system_graph& operator=(const system_graph&) = delete;
    system_graph(system_graph &&) = default;
    system_graph& operator=(system_graph &&) = default;

    ~system_graph()
    {
        graphs::rfor_each(deps, [this](entt::id_type id) {
            entities.destroy(id);
        });
    }
#pragma endregion

#pragma region System Graph
public:
    system_graph() = default;

    using dependency_map = graphs::directed_adjacency_map<entt::id_type>;

    /** Get the registry used to store systems */
    const auto& registry() const { return entities.ctx(); }

    /** Get a copy of the dependency graph */
    dependency_map dependencies() const { return deps; }

    /** Emplace a system in the graph using its constructor */
    template<typename System, typename... Args>
    System& emplace(Args &&... args)
    {
        // register any dependencies the system has declared
        declare_dependencies<System>();

        // create the entity for this subsystem
        // (destroying any subsystems associated with the type hash)

        const auto id = entt::type_hash<System>::value();
        if (entities.valid(id)) { entities.destroy(id); }
        const auto entity = entities.create(id);

        using unique_system = std::unique_ptr<System>;
        auto& system = entities.emplace<unique_system>(
                entity,
                std::make_unique<System>(std::forward<Args>(args)...));

        return *system;
    }
    /** Load a system to the graph using its static load method
     *
     * \return a pointer to the loaded system
     *
     * If the system already exists, return the existing system instead
     */
    template<typename System, typename... Args>
    requires can_load_with<System, Args...>
    System* load(Args &&... args)
    {
        if (auto* system = find<System>()) { return system; }
        return System::load(*this, std::forward<Args>(args)...);
    }

    /** Find a subsystem */
    template<typename System>
    System* find()
    {
        using unique_system = std::unique_ptr<System>;
        const auto id = entt::type_hash<System>::value();

        if (auto* system = entities.try_get<unique_system>(id)) {
            return system->get();
        }
        return nullptr;
    }
private:
    using id_inserter_t = std::insert_iterator<std::vector<entt::id_type>>;

    template<typename System>
    void declare_dependencies()
    {
        const auto system_id = entt::type_hash<System>::value();
        graphs::directed_edge_set<entt::id_type> edges;
        deps.emplace(system_id, edges);
    }

    template<typename System>
    requires has_dependencies<System, id_inserter_t>
    void declare_dependencies()
    {
        std::vector<entt::id_type> incoming;
        System::dependencies(std::back_inserter(incoming));

        const auto to = entt::type_hash<System>::value();
        graphs::add_edges_from(deps, incoming, to);
    }

    entt::basic_registry<entt::id_type> entities;
    dependency_map deps;
#pragma endregion
};
}
