// #include <nickel/nickel.hpp>

// #include <ostream>
// #include <utility>

// #include <catch2/catch.hpp>

// namespace {
//     constexpr auto dim2 = nickel::name_group(nickel::names::x, nickel::names::y);

//     auto dist2()
//     {
//         return nickel::wrap(dim2)([](double x, double y) { return std::hypot(x, y); });
//     }

//     auto dist3()
//     {
//         return nickel::wrap(nickel::kwargs_group(dim2), nickel::names::z)(
//             [](auto&& kwargs, double z) {
//                 return dist2() //
//                     .x(dist2() //
//                         (std::forward<decltype(kwargs)>(kwargs))()) //
//                     .y(z)();
//             });
//     }
// }

// TEST_CASE("kwargs works")
// {
//     double const expected = std::hypot(std::hypot(1, 2), 3);

//     auto result = dist3() //
//                       .x(1)
//                       .y(2)
//                       .z(3)();

//     CHECK(result == expected);
// }
