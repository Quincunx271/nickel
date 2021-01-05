Usage for an Ordinary Function
==============================

Motivation
----------

Suppose we have a function for computing the logarithm of its input for a particular base.
We might declare this function like so:

.. code:: c++

    double logarithm(double operand, double base);

This declaration has a flaw, there's nothing inherently obvious about the order of the parameters,
and especially because they're of the same type, it's easy for a user to accidentally interchange
arguments when calling the logarithm:

.. code:: c++

    double result = logarithm(100, 10); // log10(100) or log100(10)?

We can improve this situation by using named parameters.


Using Nickel
------------
Because C++ doesn't have reflection-based code generation yet,
we have to declare what names we want before we use them.

.. code:: c++

    // Declares a variable with the name `base_name`
    // which declares a named parameter with name `base`
    NICKEL_NAME(base_name, base);

If you want the variable to have the same name as the name, you can omit the second parameter:

.. code:: c++

    // Declares a variable with the name `base`
    // which declares a named parameter with name `base`
    NICKEL_NAME(base);


To declare a function with named parameters using Nickel,
we have to wrap a regular function with named information:

.. code:: c++

    constexpr auto logarithm(double operand) {
        return nickel::wrap(base_name)([operand](double base) {
            // Implementation.
            // Consider calling an extern logarithm_impl(...) function.
        });
    }

In the above, we have one positional parameter---``operand``---and one named
parameter---``base_name``.

Now the calling code looks like this:

.. code:: c++

    double result = logarithm(100)
                        .base(10) // Specifies the named argument "base"
                        (); // Calls the function with the specified arguments


Default Arguments
-----------------

Suppose we actually want to provide a default argument for our named parameter.
With Nickel, that is straightforward:

.. code:: c++

    constexpr auto logarithm(double operand) {
        return nickel::wrap(base_name = 10.0)([operand](double base) {
            // Implementation.
        });
    }

Now if the user doesn't pass the ``base`` argument, it defaults to ``10.0``:

.. code:: c++

    double result = logarithm(100)(); // Equivalent to setting .base(10.0)

We can even get fancier. Nickel doesn't require anything in particular about the type of arguments,
so we can provide an ``integral_constant`` for the base and swap out for an optimized default
implementation in such a case:

.. code:: c++

    constexpr auto logarithm(double operand) {
        return nickel::wrap(base_name = std::integral_constant<int, 10>{})([operand](auto base) {
            // Special case base 10:
            if (base == 10.0) {
                return std::log10(operand);
            } else {
                // Regular implementation.
            }

            // ...

            // Alternatively, we can special case only constant bases of base 10:
            if constexpr (decltype(base)::value == 10) {
                return std::log10(operand);
            } else {
                // Regular implementation.
            }
        });
    }


A More Complex Example
----------------------

Most functions generally have many parameters, not just two.
Suppose we are simulating physics for a car and need to provide a multitude of parameters:

 - Time Delta - the amount of time since the last tick.
 - Wheel Position - the angle of the steering wheel, defaulting to 0.
 - Speed - how far the car should travel per time unit
 - Direction - where the car is facing

While it would be best to encapsulate these concepts into appropriate classes, for the sake of example,
writing an un-encapsulated function with Nickel could look like this:

.. code:: c++

    NICKEL_NAME(speed, speed);
    NICKEL_NAME(steering_wheel, steering);
    NICKEL_NAME(direction, direction);

    class Car {
    public:
        constexpr auto car_tick(double time_delta) {
            return nickel::wrap(speed, steering_wheel = 0, direction)(
                [this, time_delta](double speed, double steering, double direction) {
                // Implementation
            });
        }
    };

    // ...

    // Usage
    Car c;
    c.car_tick(1.0)
        .steering(1)
        .direction(0)
        .speed(100)();
