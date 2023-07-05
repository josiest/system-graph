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

    using id_set = std::unordered_set<entt::id_type>;
    using id_inserter_t = std::insert_iterator<id_set>;

    using dependency_node = std::pair<id_set, id_set>;
    using dependency_graph =
        std::unordered_map<entt::id_type, dependency_node>;

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

    template<typename T>
    void declare_dependencies()
    {
        namespace pig = pi::graphs;
        pig::node node;
        auto& children = node.children;
        read_dependents<T>(std::inserter(children, children.begin()));
        dependencies.emplace(entt::type_hash<T>::value(), node);
    }

    template<typename T>
    requires has_dependencies<T, id_inserter_t>
    void declare_dependencies()
    {
        namespace pig = pi::graphs;
        pig::node node;
        T::dependencies(std::inserter(node.parents, node.parents.begin()));
        const auto subsystem_id = entt::type_hash<T>::value();
        for (const auto dependent_id : node.parents) {

            if (auto search = dependencies.find(dependent_id);
                     search != dependencies.end()) {

                pig::node& parent = search->second;
                parent.children.insert(subsystem_id);
            }
        }
        auto& children = node.children;
        read_dependents<T>(std::inserter(children, children.begin()));
        dependencies.emplace(subsystem_id, node);
    }

    template<typename T, std::output_iterator<entt::id_type> TypeOutput>
    TypeOutput read_dependents(TypeOutput into_types)
    {
        namespace pig = pi::graphs;
        namespace ranges = std::ranges; namespace views = std::views;

        const auto subsystem_id = entt::type_hash<T>::value();
        auto is_dependent = [=](const auto& elem) {
            return elem.second.parents.contains(subsystem_id);
        };
        return ranges::transform(dependencies | views::filter(is_dependent),
                                 into_types,
                                 &pig::graph::value_type::first).out;
    }

    entt::registry registry;
    entt::entity system_id;
    subsystem_map subsystems;
    pi::graphs::graph dependencies;
};
}
