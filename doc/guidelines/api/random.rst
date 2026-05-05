****************************
Random Domain API Guidelines
****************************

These guidelines extend the Common API Guidelines for public APIs that generate random values, sample collections, or
fill random byte buffers.

The purpose of this document is to define a base naming vocabulary for random APIs.
It is intentionally plain, technical and list based to get a quick overview of method names and their usage patterns.
If you introduce new vocabulary, update this page to provide a good reference for future extensions.

Core Semantics
==============

Generator Vocabulary
--------------------

.. code-block:: text

    Random // abstract generator interface and convenience sampling API
    FastRandom // fast pseudo-random generator for non-security use
    ThreadSafeFastRandom // synchronized fast pseudo-random generator for shared non-security use
    SecureRandom // operating-system-backed generator for security-sensitive values

Range and Empty-Input Semantics
-------------------------------

.. code-block:: text

    integer minimum/maximum // inclusive bounds, automatically ordered when reversed
    floating minimum/maximum // ordered bounds with generator-specific endpoint behavior
    zero/infinite count // produces an empty generated collection
    empty choices // selection returns fallback; generated collections are empty
    empty byte span // accepted by fillBytes()
    empty or invalid count // selectIndex() returns ElementIndex::noIndex()

Primary Types
=============

.. code-block:: text

    Random // common generator interface with selection, building, shuffling and byte-fill helpers
    FastRandom // reproducible pseudo-random generator when constructed with an explicit seed
    ThreadSafeFastRandom // mutex-protected FastRandom wrapper for shared generator instances
    SecureRandom // cryptographic generator backed by the platform entropy source

Source Patterns
===============

.. code-block:: text

    o.getInt❮width❯(minimum, maximum) -> int❮width❯_t // random signed integer in inclusive range
    o.getUInt❮width❯(minimum, maximum) -> uint❮width❯_t // random unsigned integer in inclusive range
    o.getDouble(minimum, maximum) -> double // random floating-point value in ordered range
    o.getBool() -> bool // random boolean
    o.fillBytes(destination) -> void // fill an existing byte span with random bytes

Selection Patterns
==================

.. code-block:: text

    o.selectInteger(minimum, maximum) -> T // random native integer in inclusive ordered range
    o.selectInteger(range) -> T // random native integer from an IntegerRange
    o.selectIndex(count) -> unit::ElementIndex // random index in [0, count)
    o.selectElement(choices[, valueIfEmpty]) -> T // random element or fallback for empty choices

Build Patterns
==============

.. code-block:: text

    o.buildIntegerList(count, minimum, maximum) -> util::List<T> // random integers with replacement
    o.buildString(length, characters) -> text::String // random UTF-8 string from a character set
    o.buildByteBlock(length) -> mem::ByteBlock // random byte block
    o.buildElementList(count, choices) -> util::List<T> // sample elements with replacement
    o.buildUniqueElementList(count, choices) -> util::List<T> // sample elements without replacement

Mutation Patterns
=================

.. code-block:: text

    o.shuffle(span/vector/list) -> void // randomly reorder values in place

Construction and Application Patterns
=====================================

.. code-block:: text

    FastRandom() // automatically seeded pseudo-random generator
    FastRandom(seed) // reproducible pseudo-random sequence
    ThreadSafeFastRandom() // automatically seeded shared pseudo-random generator
    ThreadSafeFastRandom(seed) // reproducible synchronized pseudo-random sequence
    SecureRandom() // generator using the system entropy source
    Application::random() -> ThreadSafeFastRandom& // shared application pseudo-random generator
    Application::secureRandom() -> SecureRandom& // shared application secure generator

Error Patterns
==============

.. code-block:: text

    SecureRandom::❮draw❯(...) // may throw err::RandomError when entropy cannot be provided
    FastRandom::❮draw❯(...) // pseudo-random draw without entropy-source errors
    ThreadSafeFastRandom::❮draw❯(...) // synchronized pseudo-random draw without entropy-source errors
