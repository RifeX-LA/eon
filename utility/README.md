# Overview

Features:

* `eon::pass_fn()` - convenient way to pass functions
* `eon::unwrap_ref<T>()` - returns stored value, if `T` is a `std::reference_wrapper`, or value itself otherwise
* `eon::unwrap_ptr<T>()` - returns stored value, if `T` is a _pointer type_, or value itself otherwise

# Requirements

C++20