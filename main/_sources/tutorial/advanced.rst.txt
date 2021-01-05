Using Nickel to Steal Members
=============================

Motivation
----------

Suppose you have an object of a custom type which is no longer needed.
You may want to repurpose some of its data members so that you can reuse their allocations,
or perhaps you really just want those few data members.
This is fine for stealing one object:

.. code:: c++

    std::string name = std::move(object).name;

But when you want to move multiple objects, this feels very wrong:

.. code:: c++

    std::string name = std::move(object).name;
    std::vector<int> ids = std::move(object).ids;

It looks like we're moving ``object`` twice!
While it is true that ``std::move(...)`` is only a cast which makes this safe,
the rule of thumb is to only ``std::move(...)`` from an object once.
This code breaks that rule, not to mention that it breaks the encapsulation on our data members.


Using Nickel
------------

As an **experimental** feature, Nickel has a ``nickel::steal(...)`` function to produce a nicer syntax.
If you use Nickel, the user would write:

.. code:: c++

    auto [name, ids] = std::move(object).steal()
                            .name()
                            .ids()();

    // Or pre-C++17:
    std::string name;
    std::vector<int> ids;
    std::tie(name, ids) = std::move(object).steal()
                            .name()
                            .ids()();

The order of the members is arbitrary; if the user reversed the order,
the returned tuple will match the user's order:

.. code:: c++

    std::string name;
    std::vector<int> ids;
    std::tie(ids, name) = std::move(object).steal()
                            .ids()
                            .name()();

The user also does not have to list all the members,
and if they list only one, there will be no tuple:

.. code:: c++

   std::string name = std::move(object).steal().name()();

To use this functionality, call ``nickel::steal(...)``:

.. code:: c++

    NICKEL_NAME(name_name, name);
    NICKEL_NAME(ids_name, ids);

    class MyCustomType {
    private:
        std::string name;
        std::vector<int> ids;

    public:
        auto steal() && {
            return nickel::steal(
                std::move(*this),
                name_name = &MyCustomType::name,
                ids_name = &MyCustomType::ids);
        }
    };
