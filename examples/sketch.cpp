#include <iostream>
#include <iterator>
#include <algorithm>
#include <array>
#include <string_view>

#include <entt/entt.hpp>
#include "pi/systems.hpp"

namespace order {
enum order {
    first, second, third, fourth
};
}
constexpr std::array<std::string_view, 4> name_for{
    "first", "second", "third", "fourth"
};

template<order::order Order>
class order_system : public pi::ISubsystem {
public:
    static order_system<Order>* load(pi::system_manager& systems)
    {
        std::cout << "load " << name_for[Order] << "\n";
        return &systems.emplace<order_system<Order>>();
    }
    virtual void destroy() override
    {
        std::cout << "destroy " << name_for[Order] << "\n";
    }
    virtual constexpr std::string_view name() const override
    {
        return name_for[Order];
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
    static second* load(pi::system_manager& systems)
    {
        systems.load<first>();
        std::cout << "load " << name_for[order::second] << "\n";
        return &systems.emplace<second>();
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
    static fourth* load(pi::system_manager& systems)
    {
        systems.load<second>();
        systems.load<third>();
        std::cout << "load " << name_for[order::fourth] << "\n";
        return &systems.emplace<fourth>();
    }
};

int main()
{
    pi::system_manager systems;
    std::cout << "\n[load]\n";
    systems.load<fourth>();

    std::cout << "\n[dependencies]\n";
    systems.print_dependencies_to(std::cout);

    pi::system_manager moved_systems = std::move(systems);
    std::cout << "\n[destroy]\n";
}
