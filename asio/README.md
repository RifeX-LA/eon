# Asio serialization

Include `<eon/asio.hpp>`. Functions are in `eon::asio` and perform synchronous
serialization over `boost::asio::ip::tcp::socket`. The module requires C++23
and Boost headers. On Windows, consumers must link the socket system libraries
(the tests link `ws2_32` and `mswsock`).

## Reading and writing

`write(socket, value)` serializes a value; `read<T>(socket)` returns a new value.
Supported round trips include integers, bool, enums, standard tuples/pairs,
variants, expected (including void), optional, default-deleter `unique_ptr`, `shared_ptr` and owning
ranges such as strings, vectors, lists, sets, maps and fixed `std::array`.
Element types must themselves be serializable and meet the target container's
construction requirements. Use `std::array` for fixed-size round trips;
native C arrays cannot be returned by value.

Custom serializers use either `value.write(socket)` on a const value plus a
mutating `value.read(socket)`, or ADL functions `write(socket, value)` and
`read(socket, value)`. Member pairs take priority over ADL pairs. These read
paths default-construct the destination before invoking its read operation.

Integer bytes are big-endian; bool is represented as an `int`. Range lengths
and variant indices are encoded as `std::size_t`. Floating-point values and
the fallback for trivially copyable types use native object representation.
Consequently, the format depends on type widths and ABI and is not a versioned
portable wire protocol.

`write_as_buffer` and `read_as_buffer<T>` explicitly use raw object bytes.
An overload of `read_as_buffer(socket, value)` fills an existing object.
Types must be trivially copyable, and the sender must provide valid
representations for the destination. Raw pointer and non-owning view
deserialization is unsupported.

Invalid variant indices, mismatched fixed sizes and overflowing range byte
counts are rejected. Dynamic range reads use a cached single-pass input view
and `std::ranges::to<T>`. These checks do not impose an application payload
quota; callers must choose limits suitable for their streams.

## Synchronous deadlines

```cpp
using namespace std::chrono_literals;
eon::asio::write_with_timeout(socket, outgoing, 500ms);
auto incoming = eon::asio::read_with_timeout<MyType>(socket, 500ms);
```

A steady-clock deadline covers all primitive transfers in one value.
No running event loop is needed: the implementation temporarily enables
nonblocking I/O, retries `would_block` at intervals of up to 1 ms, and attempts
to restore the original blocking mode on scope exit. Exclusive access to the
socket is required during the call.

Zero/negative durations throw a Boost system error with
`boost::asio::error::timed_out`; nonfinite durations throw
`std::invalid_argument`. Large finite deadlines saturate. Other socket errors
propagate. No asynchronous operations are launched or cancelled.

A timeout does not undo bytes already transferred. The caller must recover
framing or close the connection before sending/reading another frame.
Custom serializers must use the module's transport operations for the deadline
to apply; direct Boost calls, arbitrary callbacks and file operations cannot
be interrupted by these functions.

## Files

`write_file(socket, source_path)` sends binary bytes without a size prefix.
`read_file(socket, destination_path)` reads until EOF and overwrites the
destination. Shut down the sender's write direction after the file to delimit it.
Transfers use 8192-byte chunks. Open, file I/O and socket errors propagate as
exceptions. There are no file-transfer timeout overloads.

[Asio tests](../tests/asio.cpp) use local TCP connections and temporary files.
