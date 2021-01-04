Reference
=========

Standard Features
-----------------

.. PERMALINK is the second one in these pairs

.. _calling-a-nickel-wrapped-function:
.. _nickel-call:

Calling a Nickel-Wrapped Function
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Call a Nickel-wrapped function by initiating, setting arguments, then finishing the call:

.. code:: c++

    auto result = wrapped_function(positional_arg, 2) // initiate
        .param1(2)      // set args
        .param2(var)    // set args
        ();             // finish the call

.. note::

    Setting the arguments can be done in any order.
    You may also skip setting any arguments that have default arguments.

.. warning::

    Do NOT store a partial result.
    Once you initiate a function call, make sure you finish the call in the same expression.

.. _declaring-names:
.. _name-decl:

Declaring Names
^^^^^^^^^^^^^^^

Using ``NICKEL_NAME(...)`` at namespace scope:

.. code:: c++

    // Declares a variable `variable_name` which provides a
    // .method_name(...) name.
    NICKEL_NAME(variable_name, method_name);

    // The variable can have the same name as the method.
    NICKEL_NAME(name, name);

    // Shorthand for reusing the same name:
    NICKEL_NAME(name);

.. _declaring-nickel-wrapped-function:
.. _fn-decl:

Declaring Nickel-Wrapped Function
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Make a function wrapping a call to ``nickel::wrap(...)``:

.. code:: c++

    constexpr auto my_function() {
        return nickel::wrap(...)([](...) { ... });
    }

.. _declaring-named-parameters:
.. _named-param-decl:

Declaring Named Parameters
^^^^^^^^^^^^^^^^^^^^^^^^^^

Pass names to ``nickel::wrap(...)``:

.. code:: c++

    constexpr auto my_function() {
        // x_name and y previously declared as:
        // NICKEL_NAME(x_name, x);
        // NICKEL_NAME(y, y);
        return nickel::wrap(x_name, y)(
            // Order of args to nickel::wrap(...)
            // is the order that the arguments are passed to the function:
            [](auto x, auto y) {
                ...
            });
    }

.. _declaring-positional-parameters:
.. _pos-param-decl:

Declaring Positional Parameters
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Add parameters to the wrapper function and capture them in the lambda:

.. code:: c++

    constexpr auto my_function(int positional1, int const& ref) {
        return nickel::wrap(x_name, y)(
            [positional1, &ref](auto x, auto& y) {
                // ref doesn't dangle even though we captured it by reference.
                // Even if it was assigned to a temporary, that temporary will live
                // until after this lambda is called.

                // As can be seen by y's declaration, it expects to be a reference.
                // This is also supported.
                ...
            });
    }

.. _adding-default-arguments:
.. _default-args:

Adding Default Arguments
^^^^^^^^^^^^^^^^^^^^^^^^

Add default arguments with `name_variable = default_arg`:

.. code:: c++

    constexpr auto logarithm(double arg) {
        return nickel::wrap(base = 10.0)([arg](double base) {
            ...
        });
    }

.. deferred-default-arguments is the perma-target:

Deferred Default Arguments
^^^^^^^^^^^^^^^^^^^^^^^^^^

Default arguments can have their evaluation deferred with
  ``name_variable = nickel::deferred([] { return value; })``.
This will prevent evaluation if the user provides a value for the argument
  instead of relying on the default value:

.. code:: c++

    using namespace std::string_literals;

    constexpr auto say_hello() {
        return nickel::wrap(
            prefix = nickel::deferred([] { return "Some long string which doesn't fit in the SSO"s; }))
            ([arg](auto const& prefix) {
                ...
            });
    }

    ...

    // Does not pay the cost of constructing the long string
    say_hello()
        .prefix("Hello")
        ();

    // Still defaults to the default argument
    say_hello()();


Advanced Features
-----------------

.. _declaring-a-name-group:
.. _name-group:

Declaring a Name Group
^^^^^^^^^^^^^^^^^^^^^^

Collect a group of names together by calling ``nickel::name_group(x, y)``.
These can be used in place of a name variable when calling ``nickel::wrap(...)``,
which acts like each name bound separately.
Default arguments may also be specified via the name group:

.. code:: c++

    constexpr auto cartesian_names = nickel::name_group(names::x, names::y, names::z = 0);

    constexpr auto my_function() {
        return nickel::wrap(cartesian_names, names::other)(
            [](double x, double y, double z, double other) {
                ...
            });
    }

.. _using-kwargs:
.. _kwargs:

Using Kwargs
^^^^^^^^^^^^

Mark a group of names as kwargs by calling ``nickel::kwargs_group(name_group, other_name)``.
The ``kwargs`` argument will be passed as the first argument of the function passed to ``nickel::wrap(...)(...)``.
Passed parameters can be accessed via ``kwargs.get(name_variable)`` or via ``kwargs.name()``.
The ``kwargs`` can be bound to another named function by forwarding the ``kwargs`` parameter
into the initiated function with ``operator()``:

.. code:: c++

    constexpr auto my_function() {
        return nickel::wrap(nickel::kwargs_group(hash_fn_name, equal_op_name), other_name)(
            [](auto&& kwargs, auto const& other) {
                auto hash_fn = kwargs.get(hash_fn_name);
                auto also_hash_fn = kwargs.hash_fn();
                ...
                other_wrapped_function()
                    (std::forward<decltype(kwargs)>(kwargs))
                    .some_other_name(...);
            });
    }


Experimental Features
---------------------

Use these features at your own risk. They may disappear in future versions of Nickel.

.. _declaring-multivalued-parameters:
.. _multivalued-param-decl:

Declaring Multivalued Parameters
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Declare that a name should be called with multiple arguments by calling ``name.multivalued<N>()`` when passing it to ``nickel::wrap(...)``.
The ``N`` that you pass specifies the number of parameters used.
Multivalued parameters will have their argument in the lambda passed as a ``std::tuple<...>`` of references.
Passing ``N = 1`` will result in a ``std::tuple<...>`` instead of ``...``.

.. code:: c++

    constexpr auto move(Point& p) {
        return nickel::wrap(to.multivalued<2>())([&p](auto to) {
            p.x = std::get<0>(to);
            p.y = std::get<1>(to);
        });
    }

    ...

    move(point)
        .to(100, 200)
        ();

Multivalued parameters may have a default argument applied like other named parameters.
In this case, the lambda argument will be passed the default value without any ``std::tuple<...>`` conversions.
It is recommended to use a tuple-like type for default arguments on multivalued parameters to avoid special casing.


.. code:: c++

    constexpr auto move(Point& p) {
        return nickel::wrap(to.multivalued<2>() = std::make_pair(0, 0))([&p](auto to) {
            p.x = std::get<0>(to);
            p.y = std::get<1>(to);
        });
    }

    ...

    // Moves to (0, 0)
    move(point)();

.. _using-nickel-to-steal-members:
.. _nickel-steal:

Using Nickel to Steal Members
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Use ``nickel::steal(...)`` to make a function that allows members to be stolen:

.. code:: c++

    class MyCustomType {
    private:
        std::string name;
        std::vector<int> ids;
        std::unique_ptr<int> memory;

    public:
        auto steal() && {
            return nickel::steal(
                std::move(*this),
                // Each name must be given a default argument of the pointer-to-member-data
                name_name = &MyCustomType::name,
                ids_name = &MyCustomType::ids,
                memory_name = &MyCustomType::memory);
        }
    };

Currently, member access is only supported for pointer-to-member-data.

When users evaluate the ``steal()`` function, they will get a tuple of the requested members
in the order that they request the members in:

.. code:: c++

    std::unique_ptr<int> memory;
    std::vector<int> ids;
    std::tie(memory, ids) = std::move(object).steal()
        .memory()
        .ids()();

However, if they request only a single member, there will be no tuple:

.. code:: c++

    std::unique_ptr<int> memory = std::move(object).steal().memory()();
