# Overview

The library with many useful concepts. 

Available features:

* `eon::decayed<T>` - is `T` decayed?
* `eon::without_cv<T>` - does `T` haven't cv-qualifiers?
* `eon::without_ref<T>` - is `T` a non-reference type?
* `eon::without_cvref` - does `T` haven't cv-qualifiers or reference?
* `eon::ref_or_ptr<T>` - is `T` a reference or pointer type?
* `eon::exists_in<T, Types...>` - does `Types...` contain `T`?
* `eon::character<T>` - is `T` a character type?
* `eon::indirectable<T>` - does `T` have `operator*()`? 
* `eon::invocable_r<Fn, R, Args...>` - is `Fn` invocable with `Args...` and the type of `Fn`'s invoke result is _exactly_ `R`?  
* `eon::invocable_conv_r` - is `Fn` invocable with `Args...` and the type of `Fn`'s invoke result is _convertible to_ `R`
* `eon::specialization_of<T, Template>` - is `T` an any specialization of `Template`?
* `eon::derived_from_specialization_of<T, Template>` - did `T` derive from any specialization of `Template`?
* `eon::tuple_like<T>` - is `T` a tuple-like object, i.e. it has `std::tuple_size<T>` and `std::tuple_element<I, T>` with ADL-founded `get<I>` for all `I` in [0, `std::tuple_size_v<T>`)?
* `eon::pair_like<T>` - is `T` a tuple-like object with `std::tuple_size<T> == 2`?

# Requirements

C++20