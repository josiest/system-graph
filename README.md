# system-graph

system-graph is an abstraction of the Service Locator pattern, that functions as
a container of singletons with an internal graph of their dependencies.

A system graph supports declaring dependencies of a class when adding it into
the container. These dependencies are used to to iterate cuts of the graph such
that no class is visited before all of its depencies have been visited.

The system-graph library also supports reverse-iteration. The system-graph 
class uses this to call a virtual destroy method on each system to clean up
resources in an order their dependencies allow.

## requirements
- [EnTT](https://github.com/skypjack/entt)
- A compiler for C++20 or later

## example
This example can also be found in the examples folder

```cpp
#include <iostream>
#include <iterator>
#include <algorithm>
#include <array>
#include <string_view>

#include <entt/entt.hpp>
#include "pi/systems/system_graph.hpp"

namespace order {
enum order {
    first, second, third, fourth
};
}
constexpr std::array<std::string_view, 4> name_for{
    "first", "second", "third", "fourth"
};

template<order::order Order>
class order_system : public pi::ISystem {
public:
    static order_system<Order>* load(pi::system_graph& systems)
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
    static second* load(pi::system_graph& systems)
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
    static fourth* load(pi::system_graph& systems)
    {
        systems.load<second>();
        systems.load<third>();
        std::cout << "load " << name_for[order::fourth] << "\n";
        return &systems.emplace<fourth>();
    }
};

int main()
{
    pi::system_graph systems;
    std::cout << "\n[load]\n";
    systems.load<fourth>();

    std::cout << "\n[dependencies]\n";
    systems.print_dependencies_to(std::cout);

    pi::system_graph moved_systems = std::move(systems);
    std::cout << "\n[destroy]\n";
}
```

example output:
```
[load]
load first
load second
load third
load fourth

[dependencies]
fourth depends on [third, second]
third has no dependencies
second depends on [first]
first has no dependencies

[destroy]
destroy fourth
destroy third
destroy second
destroy first
```
