//          Copyright Justin Bassett 2019 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#include <nickel/nickel.hpp>

#include <type_traits>
#include <utility>

#include <catch2/catch.hpp>

namespace {
    struct dummy_fn
    {
        template <typename... Ts>
        constexpr void operator()(Ts&&...) const
        { }
    };

    template <typename...>
    struct void_t_impl
    {
        using type = void;
    };

    template <typename... Ts>
    using void_t = typename void_t_impl<Ts...>::type;

    NICKEL_NAME(abc, xyz);
    NICKEL_NAME(xyz);

    template <typename T, typename = void>
    struct has_xyz_mem : std::false_type
    { };

    template <typename T>
    struct has_xyz_mem<T,
        void_t<decltype(nickel::wrap(std::declval<T const&>())(dummy_fn {}).xyz(42))>>
        : std::true_type
    { };
}

TEST_CASE("NICKEL_NAME(name) works")
{
    STATIC_REQUIRE(has_xyz_mem<decltype(::xyz)>::value);
}

TEST_CASE("NICKEL_NAME(var, name) works")
{
    STATIC_REQUIRE(has_xyz_mem<decltype(::abc)>::value);
}
