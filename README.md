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

# how to use
The system-graph is fairly simple, yet still likely easy to get wrong. We'll
go over the necessary steps to create a system

## declaring dependencies
If your system has dependencies, it should declare them in the `dependencies`
function. This is a static function that requires a specific signature. It must
take and return an `output_iterator` of `entt::id_type`.

Within the body of the dependencies function, the hashed id types of the
dependent classes should be copied into the output iterator. The function
should then return the currently valid output iterator.

Here's an example of what this looks like:

```cpp
class fourth {
    // ...
    template<std::output_iterator<entt::id_type> TypeOutput>
    static TypeOutput dependencies(TypeOutput into_dependencies)
    {
        namespace ranges = std::ranges;
        return ranges::copy(std::array{ entt::type_hash<second>::value(),
                                        entt::type_hash<third>::value() },
                            into_dependencies).out;
    }

    // ...
};
```

If your system doesn't have any dependencies, it doesn't need to define this
function. However, if it doesn't declare any dependencies, where it appears in
iteration order is indeterminate.

## loading a system

In order to register a system with a system-graph, the system must define a
static `load` function that takes in a `pi::system_graph` reference and returns
a pointer to the loaded system.

Within the body of the load function, the system should first load its
dependencies using system-graph's `load` method. This is a template function
that will call the dependent system's static `load` functions, and emplace the
depency into the system graph.

The `pi::system_graph` class provides the `emplace` method wich constructs a
system in-place, forwarding any arguments to the constructor. The emplace method
returns a reference to the constructed system, whose address can be used as the
return value for the system's load function.

Here's an example of how this works:
```cpp
class fourth {
    // ...
    static fourth* load(pi::system_graph& systems)
    {
        systems.load<second>();
        systems.load<third>();
        std::cout << "load " << name_for[order::fourth] << "\n";
        return &systems.emplace<fourth>();
    }
    // ...
};
```

# example
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
class order_system {
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

[destroy]
destroy fourth
destroy third
destroy second
destroy first
```
