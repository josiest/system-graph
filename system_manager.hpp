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
    void load()
    {
        namespace ranges = std::ranges;
        for_each(&subsystem::load);
    }
    template<typename T>
    auto& get()
    {
        return registry.get<T>(system_id);
    }
    void destroy()
    {
        namespace views = std::views; namespace ranges = std::ranges;
        rfor_each(&subsystem::destroy);
    }
private:
    using subsystem_map = std::unordered_map<entt::id_type, subsystem*>;

    using dependency_set = std::unordered_set<entt::id_type>;
    using dependency_inserter_t = std::insert_iterator<dependency_set>;

    using dependency_node = std::pair<dependency_set, dependency_set>;
    using dependency_graph =
        std::unordered_map<entt::id_type, dependency_node>;

    enum class direction { forward, reverse };

    template<direction Direction, std::invocable<subsystem*> Visitor>
    void bfs(Visitor visit)
    {
        namespace ranges = std::ranges; namespace views = std::views;

        std::deque<dependency_graph::value_type> next;
        if constexpr (Direction == direction::forward) {
            ranges::copy_if(dependencies, std::back_inserter(next),
                            &system_manager::is_root);
        }
        else {
            ranges::copy_if(dependencies, std::back_inserter(next),
                            &system_manager::is_leaf);
        }
        std::unordered_set<entt::id_type> seen;
        ranges::transform(next, std::inserter(seen, seen.begin()),
                          &dependency_graph::value_type::first);

        auto not_seen = [this, &seen](entt::id_type id) {
            return dependencies.contains(id) and not seen.contains(id);
        };

        std::unordered_set<entt::id_type> visited;
        while (not next.empty()) {

            auto from = next.front();
            next.pop_front();

            dependency_set* parents;
            dependency_set* children;
            if constexpr (Direction == direction::forward) {
                parents = &from.second.first;
                children = &from.second.second;
            }
            else {
                parents = &from.second.second;
                children = &from.second.first;
            }

            auto find_elem = [this](const auto& id) {
                return *dependencies.find(id);
            };
            if (ranges::any_of(*parents, not_seen)) {
                ranges::transform(*parents | views::filter(not_seen),
                                  std::front_inserter(next), find_elem);
                continue;
            }
            // only visit after all parents have been visited,
            // and only visit once
            if (not visited.contains(from.first)) {
                std::invoke(visit, subsystems.at(from.first));
                visited.insert(from.first);
            }
            ranges::transform(*children | views::filter(not_seen),
                              std::back_inserter(next), find_elem);

            ranges::copy(*children, std::inserter(seen, seen.begin()));
        }
    }

    template<std::invocable<subsystem*> Visitor>
    void for_each(Visitor visit)
    {
        bfs<direction::forward>(visit);
    }

    template<std::invocable<subsystem*> Visitor>
    void rfor_each(Visitor visit)
    {
        bfs<direction::reverse>(visit);
    }

    template<typename T>
    void declare_dependencies()
    {
        dependency_node node;
        read_dependents<T>(std::inserter(node.second, node.second.begin()));
        dependencies.emplace(entt::type_hash<T>::value(), node);
    }

    template<typename T>
    requires has_dependencies<T, dependency_inserter_t>
    void declare_dependencies()
    {
        dependency_node node;
        T::dependencies(std::inserter(node.first, node.first.begin()));
        const auto subsystem_id = entt::type_hash<T>::value();
        for (const auto dependent_id : node.first) {
            if (auto search = dependencies.find(dependent_id);
                     search != dependencies.end()) {

                dependency_node& parent = search->second;
                parent.second.insert(subsystem_id);
            }
        }
        read_dependents<T>(std::inserter(node.second, node.second.begin()));
        dependencies.emplace(subsystem_id, node);
    }

    template<typename T, std::output_iterator<entt::id_type> TypeOutput>
    TypeOutput read_dependents(TypeOutput into_types)
    {
        namespace ranges = std::ranges; namespace views = std::views;
        const auto subsystem_id = entt::type_hash<T>::value();

        auto is_dependent = [=](const auto& elem) {
            const dependency_node& node = elem.second;
            return node.first.contains(subsystem_id);
        };
        return ranges::transform(dependencies | views::filter(is_dependent),
                                 into_types,
                                 &dependency_graph::value_type::first).out;
    }

    static bool is_root(const dependency_graph::value_type& elem)
    {
        return elem.second.first.empty();
    }
    static bool is_leaf(const dependency_graph::value_type& elem)
    {
        return elem.second.second.empty();
    }

    entt::registry registry;
    entt::entity system_id;
    subsystem_map subsystems;
    dependency_graph dependencies;
};
}
