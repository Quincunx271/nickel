//          Copyright Justin Bassett 2019 - 2021.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#include <nickel/nickel.hpp>

// ERROR_MATCHES: missing_eval_void.test.cpp\D*19\b

NICKEL_NAME(abc);

auto do_something()
{
    return nickel::wrap(abc)([](int abc) { (void)abc; });
}

void test()
{
    do_something().abc(2);
}
