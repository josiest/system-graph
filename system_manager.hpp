#include "graphs.hpp"

#include <cstdio>
#include <string_view>

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
class ISubsystem {
public:
    virtual void load() = 0;
    virtual void destroy() = 0;
    virtual constexpr std::string_view name() const = 0;
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
    template<std::derived_from<ISubsystem> Subsystem>
    requires std::default_initializable<Subsystem>
    void add()
    {
        auto& component = registry.emplace<Subsystem>(system_id);
        const auto subsystem_id = entt::type_hash<Subsystem>::value();
        subsystems.emplace(subsystem_id, &component);
        declare_dependencies<Subsystem>();
    }
    void load() { for_each(&ISubsystem::load); }
    void destroy() { rfor_each(&ISubsystem::destroy); }

    template<typename T>
    auto& get()
    {
        return registry.get<T>(system_id);
    }

    template<std::invocable<ISubsystem*> Visitor>
    void for_each(Visitor visit) const
    {
        namespace pig = pi::graphs;
        pig::for_each(dependencies, visit_with_subsystem(visit));
    }
    template<std::invocable<const ISubsystem*> Visitor>
    void for_each(Visitor visit) const
    {
        namespace pig = pi::graphs;
        pig::for_each(dependencies, visit_with_subsystem(visit));
    }
    template<std::invocable<ISubsystem*> Visitor>
    void rfor_each(Visitor visit) const
    {
        namespace pig = pi::graphs;
        pig::rfor_each(dependencies, visit_with_subsystem(visit));
    }
    template<std::invocable<const ISubsystem*> Visitor>
    void rfor_each(Visitor visit) const
    {
        namespace pig = pi::graphs;
        pig::rfor_each(dependencies, visit_with_subsystem(visit));
    }

    template<typename OutputStream>
    void print_dependencies_to(OutputStream& os) const
    {
        for (const auto& [id, edges] : dependencies)
        {
            os << subsystems.at(id)->name();
            if (edges.incoming.empty()) {
                os << " has no dependencies\n";
                continue;
            }
            os << " depends on [";
            std::string_view sep = "";
            for (const auto parent : edges.incoming) {
                os << sep << subsystems.at(parent)->name();
                sep = ", ";
            }
            os << "]\n";
        };
    }
private:
    using subsystem_map = std::unordered_map<entt::id_type, ISubsystem*>;
    using id_inserter_t = std::insert_iterator<std::vector<entt::id_type>>;

    template<typename T>
    void declare_dependencies()
    {
        namespace pig = pi::graphs;

        const auto subsystem_id = entt::type_hash<T>::value();
        pig::edge_set<entt::id_type> edges;
        dependencies.emplace(subsystem_id, edges);
    }

    template<typename T>
    requires has_dependencies<T, id_inserter_t>
    void declare_dependencies()
    {
        namespace pig = pi::graphs;

        std::vector<entt::id_type> incoming;
        T::dependencies(std::back_inserter(incoming));

        const auto to = entt::type_hash<T>::value();
        pig::add_edges_from(dependencies, incoming, to);
    }

    template<std::invocable<ISubsystem*> Visitor>
    auto visit_with_subsystem(Visitor visit) const
    {
        return [&ids = subsystems, visit](const entt::id_type id) {
            if (auto search = ids.find(id); search != ids.end()) {
                return std::invoke(visit, search->second);
            }
        };
    }
    template<std::invocable<const ISubsystem*> Visitor>
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
    pi::graphs::graph<entt::id_type> dependencies;
};
}
