# Overview

This library provides useful utilities to work with _nullable_ types

**_Nullable_** is a type that corresponds to serving criteria:
* default constructible
* convertible to `bool`
* has `operator*()`

Features:
* `eon::invoke_nullable()`
* `eon::invoke_nullable_ref()`

The main purpose of these functions is to avoid double checks for the case 
when the value of the function or field, which is a nullable object, 
must be obtained from a nullable object.

For example:

```c++
struct widget {
    std::optional<int> opt_value;
}

int main() {
    std::optional<widget> opt_widget;
    
    // In common scenario you must double check to use widget::opt_value
    if (opt_widget) {
        if (opt_widget->opt_value) {
            std::cout << *opt_widget->opt_value;
        }
        else {
            std::cout << "empty";
        }
    }
    else {
        std::cout << "empty";
    }
    
    // Now you can just use
    if (auto const opt_value = eon::invoke_nullable(&widget::opt_value, opt_widget)) {
        std::cout << *opt_value;
    } 
    else {
        std::cout << "empty";
    }
}
```

# Requirements

C++20