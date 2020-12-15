//          Copyright Justin Bassett 2019 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#include <nickel/nickel.hpp>

#include <memory>
#include <string>
#include <vector>

#include <catch2/catch.hpp>

namespace {
    NICKEL_NAME(name, name);
    NICKEL_NAME(pointer, pointer);
    NICKEL_NAME(numbers, numbers);

    class Person
    {
    public:
        std::string name;
        std::unique_ptr<int> pointer;
        std::vector<int> numbers;

    public:
        auto steal() &&
        {
            return nickel::steal(std::move(*this),
                ::name = &Person::name, //
                ::pointer = &Person::pointer, //
                ::numbers = &Person::numbers);
        }
    };
}

TEST_CASE("Steal works")
{
    Person person;
    person.name = "Hello, World!";
    person.pointer = std::make_unique<int>(42);
    person.numbers = std::vector<int> {1, 2, 3, 4, 5, 6};

    auto const pointer_val = person.pointer.get();
    auto const name_val = person.name;

    std::string name;
    std::unique_ptr<int> pointer;

    std::tie(pointer, name) = std::move(person)
                                  .steal() //
                                  .pointer() //
                                  .name()();

    REQUIRE(pointer.get() == pointer_val);
    CHECK(*pointer == *pointer_val);
    CHECK(name == name_val);
}

TEST_CASE("Stealing a single member will unwrap the tuple")
{
    Person person;
    person.name = "Hello, World!";
    person.pointer = std::make_unique<int>(42);
    person.numbers = std::vector<int> {1, 2, 3, 4, 5, 6};

    auto const pointer_val = person.pointer.get();

    std::unique_ptr<int> pointer = std::move(person).steal().pointer()();

    REQUIRE(pointer.get() == pointer_val);
    CHECK(*pointer == *pointer_val);
}
