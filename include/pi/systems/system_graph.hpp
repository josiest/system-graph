#include "pi/graphs/digraph.hpp"
#include <concepts>
#include <iterator>

#include <unordered_map>
#include <vector>
#include <deque>

#include <string_view>
#include <entt/entity/registry.hpp>

inline namespace pi {

class ISystem {
public:
    virtual void destroy() = 0;
    virtual constexpr std::string_view name() const = 0;
};

template<typename System, std::output_iterator<entt::id_type> TypeOutput>
constexpr bool has_dependencies =
requires(TypeOutput into_types)
{
    { System::dependencies(into_types) } -> std::same_as<TypeOutput>;
};

class system_graph;
template<typename System>
constexpr bool has_default_load = requires(system_graph& systems)
{
    { System::load(systems) } -> std::same_as<System*>;
};
template<typename System, typename Config>
constexpr bool has_config_load =
requires(system_graph& systems, const Config& config)
{
    { System::load(systems, config) } -> std::same_as<System*>;
};

class system_graph {
public:
    system_graph(const system_graph&) = delete;
    system_graph& operator=(const system_graph&) = delete;

    system_graph(system_graph &&) = default;
    system_graph& operator=(system_graph &&) = default;

    system_graph() : registry{}, system_id{ registry.create() }
    {
    }
    ~system_graph()
    {
        destroy();
        systems.clear();
        registry.clear();
    }
    template<std::derived_from<ISystem> System, typename... Args>
    System& emplace(Args &&... args)
    {
        declare_dependencies<System>();
        auto& subsystem = registry.emplace<System>(
                system_id, std::forward<Args>(args)...);

        const auto subsystem_id = entt::type_hash<System>::value();
        systems.emplace(subsystem_id, &subsystem);
        return subsystem;
    }
    template<std::derived_from<ISystem> System>
    requires has_default_load<System>
    System* load()
    {
        if (registry.any_of<System>(system_id)) {
            return registry.try_get<System>(system_id);
        }
        return System::load(*this);
    }
    template<std::derived_from<ISystem> System, typename Config>
    requires has_config_load<System>
    System* load(const Config& config)
    {
        if (registry.any_of<System>(system_id)) {
            return registry.try_get<System>(system_id);
        }
        return System::load(*this, config);
    }
    inline void destroy() { rfor_each(&ISystem::destroy); }

    template<typename System>
    auto& get()
    {
        return registry.get<System>(system_id);
    }

    template<std::invocable<ISystem*> Visitor>
    void for_each(Visitor visit) const
    {
        namespace pig = pi::graphs;
        graphs::for_each(dependencies, visit_with_system(visit));
    }
    template<std::invocable<const ISystem*> Visitor>
    void for_each(Visitor visit) const
    {
        namespace pig = pi::graphs;
        graphs::for_each(dependencies, visit_with_system(visit));
    }
    template<std::invocable<ISystem*> Visitor>
    void rfor_each(Visitor visit) const
    {
        namespace pig = pi::graphs;
        graphs::rfor_each(dependencies, visit_with_system(visit));
    }
    template<std::invocable<const ISystem*> Visitor>
    void rfor_each(Visitor visit) const
    {
        namespace pig = pi::graphs;
        graphs::rfor_each(dependencies, visit_with_system(visit));
    }

    template<typename OutputStream>
    void print_dependencies_to(OutputStream& os) const
    {
        for (const auto& [id, edges] : dependencies)
        {
            os << systems.at(id)->name();
            if (edges.incoming.empty()) {
                os << " has no dependencies\n";
                continue;
            }
            os << " depends on [";
            std::string_view sep = "";
            for (const auto parent : edges.incoming) {
                os << sep << systems.at(parent)->name();
                sep = ", ";
            }
            os << "]\n";
        };
    }
private:
    using system_map = std::unordered_map<entt::id_type, ISystem*>;
    using id_inserter_t = std::insert_iterator<std::vector<entt::id_type>>;

    template<typename System>
    void declare_dependencies()
    {
        namespace pig = pi::graphs;

        const auto subsystem_id = entt::type_hash<System>::value();
        graphs::directed_edge_set<entt::id_type> edges;
        dependencies.emplace(subsystem_id, edges);
    }

    template<typename System>
    requires has_dependencies<System, id_inserter_t>
    void declare_dependencies()
    {
        namespace pig = pi::graphs;

        std::vector<entt::id_type> incoming;
        System::dependencies(std::back_inserter(incoming));

        const auto to = entt::type_hash<System>::value();
        graphs::add_edges_from(dependencies, incoming, to);
    }

    template<std::invocable<ISystem*> Visitor>
    auto visit_with_system(Visitor visit) const
    {
        return [&ids = systems, visit](const entt::id_type id) {
            if (auto search = ids.find(id); search != ids.end()) {
                return std::invoke(visit, search->second);
            }
        };
    }
    template<std::invocable<const ISystem*> Visitor>
    auto visit_with_system(Visitor visit) const
    {
        return [&ids = systems, visit](const entt::id_type id) {
            if (auto search = ids.find(id); search != ids.end()) {
                return std::invoke(visit, search->second);
            }
        };
    }

    entt::registry registry;
    entt::entity system_id;
    system_map systems;
    graphs::directed_adjacency_map<entt::id_type> dependencies;
};
}
