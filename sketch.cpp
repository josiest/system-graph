#include <cstdio>

#include <concepts>
#include <iterator>

#include <algorithm>
#include <ranges>

#include <utility>
#include <unordered_set>
#include <unordered_map>

#include <entt/entt.hpp>

class subsystem {
public:
    virtual void load() = 0;
    virtual void destroy() = 0;
};

enum class order {
    first, second
};
template<order Order> struct name_helper {};
template<>
struct name_helper<order::first>{
    static constexpr const char * value = "first";
};
template<>
struct name_helper<order::second>{
    static constexpr const char * value = "second";
};
template<order Order>
constexpr auto name_for = name_helper<Order>::value;

template<order Order>
class order_system : public subsystem {
public:
    virtual void load() override
    {
        std::printf("load %s\n", name_for<Order>);
    }
    virtual void destroy() override
    {
        std::printf("destroy %s\n", name_for<Order>);
    }
};

using first = order_system<order::first>;
using second = order_system<order::second>;

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
        subsystems.push_back(&component);
        declare_dependencies<Subsystem>();
    }
    void load()
    {
        namespace ranges = std::ranges;
        ranges::for_each(subsystems, &subsystem::load);
    }
    void destroy()
    {
        namespace views = std::views; namespace ranges = std::ranges;
        ranges::for_each(subsystems | views::reverse, &subsystem::destroy);
    }
private:
    using dependency_set = std::unordered_set<entt::id_type>;
    using dependency_inserter_t = std::insert_iterator<dependency_set>;

    using dependency_node = std::pair<dependency_set, dependency_set>;
    using dependency_graph =
        std::unordered_map<entt::id_type, dependency_node>;

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

                dependency_node& node = search->second;
                node.second.insert(subsystem_id);
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

    entt::registry registry;
    entt::entity system_id;
    std::vector<subsystem*> subsystems;
    dependency_graph dependencies;
};

int main()
{
    system_manager systems;
    systems.add<first>();
    systems.add<second>();

    system_manager moved_systems = std::move(systems);
    moved_systems.load();
}
