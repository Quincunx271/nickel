//          Copyright Justin Bassett 2019 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#include <nickel/nickel.hpp>

#include <catch2/catch.hpp>

namespace {
    NICKEL_NAME(to, to);
}

TEST_CASE("Can take multiple arguments as parameters")
{
    int x = -1;
    int y = -1;
    auto test = [&] {
        return nickel::wrap(to.multivalued<2>())([&](auto to) {
            x = std::get<0>(to);
            y = std::get<1>(to);
        });
    };

    test().to(1, 2)();

    CHECK(x == 1);
    CHECK(y == 2);
}

TEST_CASE("Multivalued parameters can have default values")
{
    int x = -1;
    int y = -1;
    auto test = [&] {
        return nickel::wrap(to.multivalued<2>() = std::make_tuple(1, 2))([&](auto to) {
            x = std::get<0>(to);
            y = std::get<1>(to);
        });
    };

    test()();

    CHECK(x == 1);
    CHECK(y == 2);
}

TEST_CASE("Can take 0 arguments as parameters")
{
    auto test = [] {
        return nickel::wrap(to.multivalued<0>())(
            [](auto to) { return std::tuple_size<decltype(to)>::value; });
    };

    auto result = test().to()();

    CHECK(result == 0);
}
