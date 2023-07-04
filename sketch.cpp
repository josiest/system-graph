#include <cstdio>

class subsystem {
public:
    virtual void load() = 0;
    virtual void destroy() = 0;
};

class first : public subsystem {
public:
    virtual void load() override
    {
        std::printf("Load first\n");
    }
    virtual void destroy() override
    {
        std::printf("Destroy first\n");
    }
};

class second : public subsystem {
public:
    virtual void load() override
    {
        std::printf("Load second\n");
    }
    virtual void destroy() override
    {
        std::printf("Destroy second\n");
    }
};

int main()
{
    first f;
    second s;

    f.load();
    s.load();

    s.destroy();
    f.destroy();
}
