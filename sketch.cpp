#include <cstdio>

#include <iterator>
#include <algorithm>
#include <array>

#include "system_manager.hpp"
#include <entt/entt.hpp>

enum class order {
    first, second, third, fourth
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
template<>
struct name_helper<order::third>{
    static constexpr const char * value = "third";
};
template<>
struct name_helper<order::fourth>{
    static constexpr const char * value = "fourth";
};
template<order Order>
constexpr auto name_for = name_helper<Order>::value;

template<order Order>
class order_system : public pi::subsystem {
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
using third = order_system<order::third>;

class second : public order_system<order::second> {
public:

    template<std::output_iterator<entt::id_type> TypeOutput>
    static TypeOutput dependencies(TypeOutput into_dependencies)
    {
        namespace ranges = std::ranges;
        return ranges::copy(std::array{ entt::type_hash<first>::value() },
                            into_dependencies).out;
    }
};

class fourth : public order_system<order::fourth> {
    template<std::output_iterator<entt::id_type> TypeOutput>
    static TypeOutput dependencies(TypeOutput into_dependencies)
    {
        namespace ranges = std::ranges;
        return ranges::copy(std::array{ entt::type_hash<second>::value(),
                                        entt::type_hash<third>::value() },
                            into_dependencies).out;
    }
};

int main()
{
    pi::system_manager systems;
    systems.add<first>();
    systems.add<second>();
    systems.add<third>();
    systems.add<fourth>();

    pi::system_manager moved_systems = std::move(systems);
    moved_systems.load();
}
