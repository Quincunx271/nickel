Nickel
======

Nickel provides an alternative named parameter syntax for function calls.

.. See https://stackoverflow.com/q/56195758/ on a TOC with different depths for entries

.. toctree::
   :caption: Contents:

   motivation.rst

.. toctree::
   :maxdepth: 2

   tutorial/index

.. toctree::
   :maxdepth: 3

   reference/index

.. toctree::
   :maxdepth: 2

   comparison


Example
=======

.. code:: c++

   #include <nickel/nickel.hpp>

   // Declare the names
   NICKEL_NAME(width_name, width);
   NICKEL_NAME(height_name, height);

   // Declare the function
   auto rectangle_area() {
      return nickel::wrap(width_name, height_name)([](int width, int height) {
         // The actual definition of the function
         return width * height;
      });
   }

   int foo() {
      // Call the function
      return rectangle_area()
         .width(42)      // Bind named arguments
         .height(35)();  // Bind `height`, then perform the actual call
      // Returns 42 * 35 = 1470
   }


Indices and tables
==================

* :ref:`genindex`
* :ref:`search`
