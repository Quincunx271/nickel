Comparison With Other Named Parameters Solutions
================================================

Nickel is far from the first solution to named parameters for C++.
How does it stack up vs. its competitors?

.. Green: 🟩
.. Yellow: 🟨
.. Orange: 🟧
.. Red: 🟥

==========================  ===========  ============  ==================  ===========  ==============  =========
Method                      C++ Std.     Compile Time  Macro Use           Last Update  Maturity        Usability
==========================  ===========  ============  ==================  ===========  ==============  =========
Nickel                      C++14        🟨 Moderate   Name decl.           2020         Immature        Good
`Designated Initializers`_  C++20*       🟩 Fast        No                  --           Yes             Decent
`Named Parameters Idiom`_   C++98        🟩 Fast        No                  --           Yes             Poor
`Boost Parameters`_         C++98        🟥 Slow       Name & Func. decl.   2020***      Yes             Good
`Argo`_                     C++14-ext**  Unknown       No                  2018         Experiment      Excellent
`cpp_named_parameters`_     C++11        Unknown       Func. call          2018         Unknown         Decent
==========================  ===========  ============  ==================  ===========  ==============  =========

| \*   Designated Initializers may use standards prior to C++20 for GCC and Clang via their extensions.
| \**  Argo uses a non-standard string literal extension which can be reimplemented to work in C++20.
| \*** Boost is receiving constant updates, so Boost Parameters is functionally still being maintained.

==========================  ============  ============  ============  ====================  =========
Method                      Out-of-order  Templ. Usage  Default Args  "Discoverable" Args*  Overloads
==========================  ============  ============  ============  ====================  =========
Nickel                      ✅            ✅            ✅            ✅                    Unknown
`Designated Initializers`_  ❌            ❌            ✅            ✅                    ❌
`Named Parameters Idiom`_   ✅            ✅ / ❌       ✅            ✅                    ❌
`Boost Parameters`_         ✅            ✅            ✅            ❌                    Unknown
`Argo`_                     ✅            ✅            ✅            ❌                    Unknown
`cpp_named_parameters`_     ✅            ✅            ❌            ❌                    Unknown
==========================  ============  ============  ============  ====================  =========

| \* "Discoverable" meaning friendly to IDE Intellisense.

Designated Initializers
-----------------------

.. code:: c++

    struct args_t {
        int x;
        int y;
    };

    void my_function(args_t);

    ...

    // Usage:
    my_function({ .x = 42, .y = 34 });

There are many blog posts about this approach, including:
 - https://pdimov.github.io/blog/2020/09/07/named-parameters-in-c20/
 - https://brevzin.github.io/c++/2019/12/02/named-arguments/

This works for simple cases, but breaks down as soon as templates enter the picture.

TODO: Show some proof of this.

Named Parameters Idiom
----------------------

.. code:: c++

    // Set up a "Builder Pattern" (Not the GoF Builder) to make something like this valid:
    my_function()
        .x(42)
        .y(34)
        .run();

    // Alternative:
    my_function(args().x(42).y(34));
    
The main disadvantage here is that it's a lot of boilerplate to write out a new fluent interface
every time you want to define a new function / set of arguments.
This is most commonly used for constructing a type, in which case it may be worth it,
but to have to write a new fluent named parameter type for each function is too much.

This solution also has a problem: for the most straightforward implementation with
a single ``fluent_interface_t`` builder type, the user can specify the same arguments multiple times
or forget to specify an argument:

.. code:: c++

    my_function()
        .x(42)
        .x(43)
        .run();

This can be solved with more code and special care.
In fact, generalizing that extra code and care into a library is the inspiration for Nickel.

Boost Parameters
----------------

https://www.boost.org/doc/libs/1_75_0/libs/parameter/doc/html/index.html

...

Argo
----

https://github.com/rmpowell77/LIAW_2017_param

...

cpp_named_parameters
--------------------

https://github.com/mserdarsanli/cpp_named_parameters

...
