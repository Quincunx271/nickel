Motivation
==========

Why Named Parameters?
---------------------

Sometimes, positional function parameters are not sufficient.
We reach situations where it's not clear what a particular argument means in a function call:

.. code:: c++

    setColor(10, 20, 30); // Is this RGB? HSV? Some other sequence?
    argparser.addArgument("--depth", false, type<int>, "The number of commits to clone");
    logarithm(10, 2); // Is this log2(10) or log10(2)?


There are techniques to work around this, but they aren't always ideal:

.. code:: c++

    setColorRGB(10, 20, 30);
    setColor(Red(10), Green(20), Blue(30));

    argparser.addArgument("--depth", required::no, "The number of commits to clone");
    // ...
    int depth = parsedArgs["--depth"]; // replaces type<int>

    // ... How do we do logarithm?
    LogArgs args;
    args.operand = 2;
    args.base = 2;
    logarithm(args); // yuck


Named parameters allow us to solve these problems.
They might not always be the right solution, but they're another tool in the toolbox:

.. code:: c++

    setColor(red=10, green=20, blue=30);
    argparser.addArgument(
        "--depth",
        required=false,
        type=type<int>,
        help="The number of commits to clone"
    );
    logarithm(2, base=10);

Unfortunately, C++ does not have this.


Why Nickel?
-----------

For a more in-depth comparison of Nickel verses the alternatives, see :doc:`comparison`.

There are many techniques to emulate named parameters in C++, but they all have flaws:
failing to work well with templates,
requiring lots of macro use,
and failing to provide parameter names in IDE intellisense
among other issues.
Nickel attempts to address these flaws, with a couple of minor features as the cherry on top.

Templates
+++++++++

Some promising named parameter solutions---designated initializers in particular---fail to work for
function templates, only working when you know the concrete type.
This is not acceptable: either we cannot generalize functions which have named parameters
or we have to have a distinct named parameter syntax just for templates.

Nickel was designed specifically to accommodate templates just as well as concrete functions.
For example, default arguments can be a completely different type from the eventually-bound value.

Macros and Boilerplate
++++++++++++++++++++++

Several named parameter solutions mangle your code by requiring extensive use of macros.
Some require that your function's declaration be replaced with a complicated macro invocation,
and others require that your function be implemented and called as a macro.
The latter is clearly undesirable, and the former still has the drawback of masking the actual
C++ tokens behind layers of macros which don't even look like a function declaration.

Other macro-free solutions require extensive boilerplate to use:

 - Designated initializers require a new struct definition for each set of function arguments.
 - The Named Parameters Idiom requires setting up a new fluent interface for each set of function arguments.

While a macro-free world would be ideal and would indeed be possible with generative reflection,
Nickel's compromise is to require a macro only to declare the names as used by the author of the function.
This provides minimal boilerplate while simultaneously avoiding the worst of macros.

IntelliSense Friendliness
+++++++++++++++++++++++++

Most named parameter solutions focus purely on the calling syntax,
but fail to account for the actual writing of the function call:
how does one determine what parameters are available or still remain to be specified?

One popular technique is to provide name objects with overloaded ``operator=`` so as to imitate an ideal named parameter syntax:

.. code:: c++

    using namespace parameter_names;
    argparser.addArgument("--depth", required_=false, type_=type<int>, help_="...");

Setting aside the issue of namespace pollution (``required_``, ``type_``, and ``help_`` are all names which are now reserved for the parameter name),
this is problematic because we don't get much IDE support when writing this function call:

.. code:: c++

    argparser.addArgument(
        "--depth",
        // Was it "required" or "required_" or "default_=value" or ...?
        // Was it "help" or "help_" or "description" or ...?
    );

IntelliSense cannot help us here, because the IDE does not know that the names are different from ordinary variables.

In contrast, for Nickel's approach, the IDE has plenty of information on what names are available and on what names have already been set:

.. code:: c++

    argparser.addArgument("--depth")
        .required(false)
        . // IDE shows exactly the names: type(...), description(...)


Nickel is not the only solution with IntelliSense friendliness (e.g. designated initializers),
but other known ways miss out on other features.


Why Nickel's Weird Syntax?
--------------------------

Nickel's calling syntax is, admittedly, weird.
However, a lot of thought was put into this specific syntax,
and it was the best that we could come up with.

.. code:: c++

    // IntelliSense problems, namespace pollution:
    argparser.addArgument(
        "--depth",
        required_=false,
        type_=type<int>,
        help_="The number of commits to clone"
    );

    // Each set of function arguments needs a new name in our namespace,
    // or we'd otherwise have invalid name suggestions which don't apply
    // for the particular function we're calling
    argparser.addArgument(
        "--depth",
        argparse::names()
            .required(false)
            .type(type<int>)
            .help("The number of commits to clone")
    );

    // How do we know when the user has stopped specifying arguments?
    // Either we lose the ability to specify arguments out of order,
    // or we lose the ability to have default arguments.
    argparser.addArgument("--depth")
        .required(false)
        .type(type<int>)
        .help("The number of commits to clone");

    // Nickel's solution: require an explicit "execute the command"
    argparser.addArgument("--depth")
        .required(false)
        .type(type<int>)
        .help("The number of commits to clone")
        (); // Execute the function.
