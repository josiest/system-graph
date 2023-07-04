#include <cstdio>
#include <concepts>
#include <algorithm>
#include <ranges>
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

class system_manager {
public:
    system_manager() : registry{}, system_id{ registry.create() }
    {
    }
    template<std::derived_from<subsystem> Subsystem>
    requires std::default_initializable<Subsystem>
    void add()
    {
        auto& component = registry.emplace<Subsystem>(system_id);
        subsystems.push_back(&component);
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
    entt::registry registry;
    entt::entity system_id;
    std::vector<subsystem*> subsystems;
};

int main()
{
    system_manager systems;
    systems.add<first>();
    systems.add<second>();
    systems.load();
    systems.destroy();
}
