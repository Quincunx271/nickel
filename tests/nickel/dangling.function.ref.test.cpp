#include <nickel/nickel.hpp>

#include <functional>

#include <catch2/catch.hpp>

namespace {
    NICKEL_NAME(foo, foo);
    NICKEL_NAME(bar, bar);

    template <typename F>
    auto frobnicate(F&& callable)
    {
        return nickel::wrap(foo, bar)([&callable](auto& foo, auto&& bar) {
            return foo = callable(bar); //
        });
    }

    std::function<int(int)> call_me()
    {
        static std::function<int(int)> const fn = [](int n) { return 42 + n; };

        return fn;
    }
}

TEST_CASE("stdfunction segfault")
{
    int x = 0;
    int result = ::frobnicate(call_me()) //
                     .foo(x)
                     .bar(-42)();

    CHECK(x == result);
    CHECK(result == 0);
}
