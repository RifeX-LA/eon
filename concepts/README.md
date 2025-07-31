# Overview

The library with many useful concepts. 

Available features:

* `eon::decayed<T>` - is `T` decayed?
* `eon::without_cv<T>` - does `T` haven't cv-qualifiers?
* `eon::without_ref<T>` - is `T` a non-reference type?
* `eon::without_cvref` - does `T` haven't cv-qualifiers or reference?
* `eon::exists_in<T, Types...>` - does `Types...` contain `T`?
* `eon::character<T>` - is `T` a character type?
* `eon::indirectable<T>` - does `T` have `operator*()`? 
* `eon::invocable_r<Fn, R, Args...>` - does `Fn` invocable with `Args...` and the type of `Fn`'s invoke result is _exactly_ `R`?  
* `eon::invocable_conv_r` - does `Fn` invocable with `Args...` and the type of `Fn`'s invoke result is _convertible to_ `R`
* `eon::specialization_of<T, Template>` - is `T` is any specialization of `Template`?
* `eon::derived_from_specialization_of<T, Template>` - is `T` is derived from any specialization of `Template`?

# Requirements

C++20