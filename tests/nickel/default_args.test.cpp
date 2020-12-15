#include <nickel/nickel.hpp>

#include <ostream>
#include <utility>

#include <catch2/catch.hpp>

namespace {
    template <int V>
    constexpr auto int_c = std::integral_constant<int, V> {};

    NICKEL_NAME(base, base);

    auto my_log(double operand)
    {
        return nickel::wrap(base = int_c<10>)([operand](auto base) {
            if (base == int_c<10>) {
                return std::log10(operand);
            } else {
                return std::log(operand) / std::log((double)base);
            }
        });
    }
}

TEST_CASE("Default arguments work")
{
    // Volatile to guarantee we don't have certain optimizations
    volatile double operand = 0.320;
    volatile double base10 = 10;
    volatile double base2 = 2;

    CHECK(std::log10(operand) != std::log(operand) / std::log(base10));
    CHECK(std::log2(operand) != std::log(operand) / std::log(base2));

    CHECK(::my_log(operand)() == std::log10(operand));
    CHECK(::my_log(operand).base(2)() == std::log(operand) / std::log(2));
}

TEST_CASE("Deferred default arguments remain unevaluated if unused")
{
    int modify_checker = 0;

    auto test = [&] {
        // Note: Very poor practice. Don't modify things in deferred default values.
        return nickel::wrap(base = nickel::deferred([modify = &modify_checker] {
            *modify = 1;
            return 1;
        }))([](int value) {
            return value; //
        });
    };

    auto result = test().base(42)();
    CHECK(result == 42);
    CHECK(modify_checker == 0);

    modify_checker = 0;
    result = test()();
    CHECK(result == 1);
    CHECK(modify_checker == 1);
}
