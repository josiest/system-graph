#include <cstdio>
#include <concepts>
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
    system_manager() : subsystems{}, system_id{ subsystems.create() }
    {
    }
    template<std::derived_from<subsystem> Subsystem>
    requires std::default_initializable<Subsystem>
    void add()
    {
        subsystems.emplace<Subsystem>(system_id);
    }
    entt::registry subsystems;
    entt::entity system_id;
};

int main()
{
    system_manager sys;
    sys.add<first>();
    sys.add<second>();

    sys.subsystems.get<first>(sys.system_id).load();
    sys.subsystems.get<second>(sys.system_id).load();

    sys.subsystems.get<second>(sys.system_id).destroy();
    sys.subsystems.get<first>(sys.system_id).destroy();
}
