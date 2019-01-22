#include <nickel/nickel.hpp>

#include <utility>

#include <catch2/catch.hpp>

namespace {
    NICKEL_NAME(foo_name, foo);
    NICKEL_NAME(bar_name, bar);

    auto some_function()
    {
        return nickel::wrap(foo_name, bar_name)([](auto&& foo, auto&& bar) {
            return std::forward<decltype(foo)>(foo)(
                std::forward<decltype(bar)>(bar));
        });
    }
}

TEST_CASE("preserves value category")
{
    int mut_lvalue = 42;

    auto result = some_function() //
                      .foo([](int& i) { return i; })
                      .bar(mut_lvalue)();

    CHECK(result == mut_lvalue);
}
