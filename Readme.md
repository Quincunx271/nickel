# Nickel

Nickel enables users to provide an alternative calling syntax allowing named parameters.

There are currently a number of solutions to named parameters in the language,
all of which have their own drawbacks.
Nickel provides a different solution with different drawbacks.

## Example

```c++
#include <nickel/nickel.hpp>

NICKEL_NAME(width_name, width);
NICKEL_NAME(height_name, height);

auto rectangle_area() {
    return nickel::wrap(width_name, height_name)([](int width, int height) {
        return width * height;
    });
}

int foo() {
    // Returns 42 * 35 = 1470
    return rectangle_area()
        .width(42)
        .height(35)();
}
```

## Documentation

See the documentation at: https://quincunx271.github.io/nickel/
