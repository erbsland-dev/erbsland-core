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

Interface
=========

.. doxygenclass:: erbsland::util::EnumFlags
    :members:
.. doxygenfunction:: erbsland::util::combineHash(const std::size_t hash1, const std::size_t hash2) noexcept -> std::size_t

.. doxygenfunction:: erbsland::util::advanceHash(std::size_t &hash, const T &arg) noexcept

.. doxygenfunction:: erbsland::util::createHash(const T1 &arg1, const Rest &...rest) noexcept -> std::size_t
.. doxygenenum:: erbsland::util::LoopResult
.. doxygenenum:: erbsland::util::LoopStatus
