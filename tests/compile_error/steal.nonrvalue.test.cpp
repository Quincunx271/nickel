//          Copyright Justin Bassett 2019 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#include <nickel/nickel.hpp>
#include <utility>

NICKEL_NAME(member_name, member);

class MyClass
{
public:
    int member;

    auto steal() &&
    {
        return nickel::steal(*this, member_name = &MyClass::member);
    }
};

int test()
{
    MyClass obj;
    return std::move(obj).steal().member()();
}
