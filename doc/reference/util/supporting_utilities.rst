.. index::
    single: Supporting Utilities

********************
Supporting Utilities
********************

Introduction
============

Enum Flags
----------

:cpp:class:`EnumFlags <erbsland::util::EnumFlags>` stores flag bits from a scoped enum in a small value object.
Use it when a public API needs a set of independent flags but should not accept arbitrary integers by accident.

Define flag enums as ``enum class`` values with an unsigned underlying type.
When you want to use the complement operator, add an ``All`` entry that contains every valid bit.
The complement is then bounded to ``All`` instead of the whole underlying integer type.

Basic Usage
~~~~~~~~~~~

Prefer defining a small alias and an explicit free ``operator|`` next to the enum:

.. code-block:: cpp

    enum class Mode : uint8_t {
        Read = 1U << 0U,
        Write = 1U << 1U,
        Execute = 1U << 2U,
        All = (1U << 0U) | (1U << 1U) | (1U << 2U),
    };

    using Modes = el::EnumFlags<Mode>;

    [[nodiscard]] constexpr auto operator|(Mode left, Mode right) noexcept -> Modes {
        return Modes{left} | right;
    }

    auto modes = Mode::Read | Mode::Write;
    modes.set(Mode::Execute);

Raw Values
~~~~~~~~~~

Use :cpp:func:`toRawValue() <erbsland::util::EnumFlags::toRawValue>` and
:cpp:func:`fromRawValue() <erbsland::util::EnumFlags::fromRawValue>` at boundaries where raw bits are required.
Raw construction preserves all bits, including bits outside ``All``.
This makes raw import explicit and avoids silently changing data from an external source.

Named Flag Wrappers
~~~~~~~~~~~~~~~~~~~

When a flag set needs domain-specific methods, define a named class using the optional CRTP parameter.
Inherit the Core constructors so the wrapper retains the regular single-flag and initializer-list syntax:

.. code-block:: cpp

    class Modes : public el::EnumFlags<Mode, Modes> {
        using Base = el::EnumFlags<Mode, Modes>;

    public:
        using Base::Base;

        auto toString() const -> el::String;
    };

The derived class must be nothrow default-constructible.
Bitwise operators, compound assignments and ``fromRawValue()`` return the derived type, so domain-specific methods
remain available on expression results.

Hash Helper
-----------

The hash helpers implement the small combining pattern used by Erbsland Core value types.
:cpp:func:`combineHash() <erbsland::util::combineHash>` merges two already-computed hash values.
:cpp:func:`advanceHash() <erbsland::util::advanceHash>` hashes one argument with ``std::hash`` and combines it with the
current hash value.

:cpp:func:`createHash() <erbsland::util::createHash>` starts with ``0`` and sequentially advances the hash for all
provided arguments.
Use it when a type's hash depends on more than one stored value.

Only :cpp:func:`combineHash() <erbsland::util::combineHash>` is a compile-time helper.
:cpp:func:`advanceHash() <erbsland::util::advanceHash>` and :cpp:func:`createHash() <erbsland::util::createHash>` call
``std::hash``, whose call operator is not required to be ``constexpr`` in C++20.

CoGenerator
-----------

:cpp:class:`CoGenerator <erbsland::util::CoGenerator>` is a small C++20 coroutine generator for lazily yielding a
sequence of values.
It is intended for internal algorithms and compact public helpers where a full container would add unnecessary storage
or control-flow noise.

Generators are move-only and single-pass.
Consume one generator either with :cpp:func:`next() <erbsland::util::CoGenerator::next>` or with range iteration, but do
not mix both styles for the same instance.
Exceptions that escape from the coroutine body are rethrown when the consumer advances the generator.

Use :cpp:func:`next() <erbsland::util::CoGenerator::next>` when yielded values must be moved out of the coroutine frame,
for example for move-only values.
Range iteration exposes yielded values as ``const`` references while the iterator is positioned at the value.

CoTask
------

:cpp:class:`CoTask <erbsland::util::CoTask>` represents one eagerly started coroutine result. It is move-only and has
one consumer.
Poll ``isComplete()`` when integrating with non-coroutine code, inspect a completed value through ``result()``, move it
out through ``takeResult()``, or await and consume the task as an rvalue.

``CoTask::run()`` moves bounded work onto the process-wide coroutine worker service.
This service is separate from stream native-I/O workers, and continuations resume without caller-thread affinity.
Exceptions are stored and rethrown by result retrieval or ``co_await``.
Destroying an incomplete task requests cancellation; an already running callable finishes normally, but a cancelled
Erbsland coroutine does not continue past its next awaited completion.

CoAsyncGenerator
----------------

:cpp:class:`CoAsyncGenerator <erbsland::util::CoAsyncGenerator>` is the asynchronous counterpart to ``CoGenerator``.
Its producer starts lazily and may combine ``co_yield`` with ``co_await``.
Each ``co_await generator.next()`` produces an ``optional`` value, with an empty optional marking completion.

The generator is move-only and single-pass, and only one ``next()`` may be outstanding.
Exceptions from its producer are rethrown by ``next()``.
Use this type for parser, tokenizer, or stream sequences whose producer must suspend between values; keep
``CoGenerator`` for simple synchronous pull sequences.

Results With Data
-----------------

:cpp:class:`ResultWithData <erbsland::util::ResultWithData>` combines the success/failure state of
:cpp:class:`Result <erbsland::util::Result>` with a typed payload. Its optional second template argument selects the
``Result`` -derived status base, preserving the specialized states and predicates while exposing data through ``data()``
and ``takeData()``.

Interface
=========

.. doxygenclass:: erbsland::util::CoAsyncGenerator
    :members:
.. doxygenclass:: erbsland::util::CoGenerator
    :members:
.. doxygenclass:: erbsland::util::CoTask
    :members:
.. doxygenclass:: erbsland::util::EnumFlags
    :members:
.. doxygenfunction:: erbsland::util::combineHash(const std::size_t hash1, const std::size_t hash2) noexcept -> std::size_t

.. doxygenfunction:: erbsland::util::advanceHash(std::size_t &hash, const T &arg) noexcept

.. doxygenfunction:: erbsland::util::createHash(const T1 &arg1, const Rest &...rest) noexcept -> std::size_t
.. doxygenenum:: erbsland::util::LoopResult
.. doxygenenum:: erbsland::util::LoopStatus
.. doxygenclass:: erbsland::util::Result
    :members:
.. doxygenclass:: erbsland::util::ResultWithData
    :members:
