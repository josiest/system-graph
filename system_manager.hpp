#include "id_graph.hpp"

#include <cstdio>

#include <concepts>
#include <iterator>

#include <algorithm>
#include <ranges>

#include <utility>
#include <unordered_set>
#include <unordered_map>
#include <deque>

#include <entt/entt.hpp>

namespace pi {
class subsystem {
public:
    virtual void load() = 0;
    virtual void destroy() = 0;
};

template<typename T, std::output_iterator<entt::id_type> TypeOutput>
constexpr bool has_dependencies =
requires(TypeOutput into_types)
{
    { T::dependencies(into_types) } -> std::same_as<TypeOutput>;
};

class system_manager {
public:
    system_manager(const system_manager&) = delete;
    system_manager& operator=(const system_manager&) = delete;

    system_manager(system_manager &&) = default;
    system_manager& operator=(system_manager &&) = default;

    system_manager() : registry{}, system_id{ registry.create() }
    {
    }
    ~system_manager()
    {
        destroy();
        subsystems.clear();
        registry.clear();
    }
    template<std::derived_from<subsystem> Subsystem>
    requires std::default_initializable<Subsystem>
    void add()
    {
        auto& component = registry.emplace<Subsystem>(system_id);
        subsystems.emplace(entt::type_hash<Subsystem>::value(), &component);
        declare_dependencies<Subsystem>();
    }
    void load() { for_each(&subsystem::load); }
    void destroy() { rfor_each(&subsystem::destroy); }

    template<typename T>
    auto& get()
    {
        return registry.get<T>(system_id);
    }

    template<std::invocable<subsystem*> Visitor>
    void for_each(Visitor visit) const
    {
        namespace pig = pi::graphs;
        pig::for_each(dependencies, visit_with_subsystem(visit));
    }
    template<std::invocable<const subsystem*> Visitor>
    void for_each(Visitor visit) const
    {
        namespace pig = pi::graphs;
        pig::for_each(dependencies, visit_with_subsystem(visit));
    }
    template<std::invocable<subsystem*> Visitor>
    void rfor_each(Visitor visit) const
    {
        namespace pig = pi::graphs;
        pig::rfor_each(dependencies, visit_with_subsystem(visit));
    }
    template<std::invocable<const subsystem*> Visitor>
    void rfor_each(Visitor visit) const
    {
        namespace pig = pi::graphs;
        pig::rfor_each(dependencies, visit_with_subsystem(visit));
    }

private:
    using subsystem_map = std::unordered_map<entt::id_type, subsystem*>;
    using id_inserter_t = std::insert_iterator<std::vector<entt::id_type>>;

    template<typename T>
    void declare_dependencies()
    {
        namespace pig = pi::graphs;
        pig::edge_set edges;
        dependencies.emplace(entt::type_hash<T>::value(), edges);
    }

    template<typename T>
    requires has_dependencies<T, id_inserter_t>
    void declare_dependencies()
    {
        std::vector<entt::id_type> incoming;
        T::dependencies(std::back_inserter(incoming));

        namespace pig = pi::graphs;
        const auto to = entt::type_hash<T>::value();
        pig::add_edges_from(dependencies, incoming, to);
    }

    template<std::invocable<subsystem*> Visitor>
    auto visit_with_subsystem(Visitor visit) const
    {
        return [&ids = subsystems, visit](const entt::id_type id) {
            if (auto search = ids.find(id); search != ids.end()) {
                return std::invoke(visit, search->second);
            }
        };
    }
    template<std::invocable<const subsystem*> Visitor>
    auto visit_with_subsystem(Visitor visit) const
    {
        return [&ids = subsystems, visit](const entt::id_type id) {
            if (auto search = ids.find(id); search != ids.end()) {
                return std::invoke(visit, search->second);
            }
        };
    }

    entt::registry registry;
    entt::entity system_id;
    subsystem_map subsystems;
    pi::graphs::graph dependencies;
};
}
