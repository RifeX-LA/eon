# Overview

This library provides several scope guards classes.

Features:

* `eon::scope_guard` - the simplest and fastest scope guard
* `eon::cancelable_scope_guard` - supports methods `reset()` and `release()`
* `eon::scope_guard_container` - provides an ability to store functions in container and contiguously invoke them.
Also supports methods `reset()` and `release()`

# Requirements

C++23