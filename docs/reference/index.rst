Reference
=========

Declaring Names
---------------

Using ``NICKEL_NAME(...)`` at namespace scope:

.. code:: c++

    // Declares a variable `variable_name` which provides a
    // .method_name(...) name.
    NICKEL_NAME(variable_name, method_name);

    // The variable can have the same name as the method.
    NICKEL_NAME(name, name);


Declaring Nickel-Wrapped Function
---------------------------------

Make a function wrapping a call to ``nickel::wrap(...)``:

.. code:: c++

    constexpr auto my_function() {
        return nickel::wrap(...)([](...) { ... });
    }


Declaring Named Parameters
--------------------------

Pass names to ``nickel::wrap(...)``:

.. code:: c++

    constexpr auto my_function() {
        // x_name and y previously declared as:
        // NICKEL_NAME(x_name, x);
        // NICKEL_NAME(y, y);
        nickel::wrap(x_name, y)(
            // Order of args to nickel::wrap(...)
            // is the order that the arguments are passed to the function:
            [](auto x, auto y) {
            ...
        });
    }


Declaring Positional Parameters
-------------------------------

Add parameters to the wrapper function and capture them in the lambda:

.. code:: c++

    constexpr auto my_function(int positional1, int const& ref) {
        nickel::wrap(x_name, y)(
            [positional1, &ref](auto x, auto& y) {
            // ref doesn't dangle even though we captured it by reference.
            // Even if it was assigned to a temporary, that temporary will live
            // until after this lambda is called.

            // As can be seen by y's declaration, it expects to be a reference.
            // This is also supported.
            ...
        });
    }


Adding Default Arguments
------------------------

Add default arguments with `name_variable = default_arg`:

.. code:: c++

    constexpr auto logarithm(double arg) {
        nickel::wrap(base = 10.0)([arg](double base) {
            ...
        });
    }


Calling a Nickel-Wrapped Function
---------------------------------

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


Declaring a Name Group
----------------------

...

Using Kwargs
------------

...
