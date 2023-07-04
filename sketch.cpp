#include <cstdio>

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

int main()
{
    first a;
    second b;

    a.load();
    b.load();

    b.destroy();
    a.destroy();
}
