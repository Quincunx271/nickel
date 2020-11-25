#include <nickel/nickel.hpp>

#include <ostream>
#include <utility>

#include <catch2/catch.hpp>

namespace {
    NICKEL_NAME(x, x);
    NICKEL_NAME(y, y);
    NICKEL_NAME(z, z);

    constexpr auto dimX = nickel::name_group(x);

    constexpr auto dim2 = nickel::name_group(dimX, y);

    auto dist2()
    {
        return nickel::wrap(dim2)([](double x, double y) { return std::hypot(x, y); });
    }

    auto dist3()
    {
        return nickel::wrap(dim2, z)(
            [](double x, double y, double z) { return std::hypot(dist2().x(x).y(y)(), z); });
    }
}

TEST_CASE("name_group works")
{
    double const expected = std::hypot(std::hypot(1, 2), 3);

    auto result = dist3() //
                      .x(1)
                      .y(2)
                      .z(3)();

    CHECK(result == expected);
}
