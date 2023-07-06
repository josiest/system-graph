#include <cstdio>

#include <iterator>
#include <algorithm>
#include <array>

#include "system_manager.hpp"
#include <entt/entt.hpp>

#include <iostream>

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
class order_system : public pi::ISubsystem {
public:
    virtual void load() override
    {
        std::cout << "load " << name_for<Order> << "\n";
    }
    virtual void destroy() override
    {
        std::cout << "destroy " << name_for<Order> << "\n";
    }
    virtual constexpr std::string_view name() const override
    {
        return name_for<Order>;
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
public:
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
    systems.add<fourth>();
    systems.add<first>();
    systems.add<third>();
    systems.add<second>();

    std::cout << "\n[dependencies]\n";
    systems.print_dependencies_to(std::cout);

    std::cout << "\n[load]\n";
    pi::system_manager moved_systems = std::move(systems);
    moved_systems.load();

    std::cout << "\n[destroy]\n";
}
