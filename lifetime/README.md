# Overview

This is a lifetime class for check if object creation, destruction, assignments and other operations took place.

**Example**

```c++
#include <eon/lifetime.hpp>

int main() {
    eon::lifetime l1;
    auto l2 = l1;

    return 0;
}

```

Output
```
lifetime() [default constructor]
lifetime(lifetime const &) [copy constructor]
~lifetime(0) [destructor]
~lifetime(0) [destructor]
```

# Requirements

C++23