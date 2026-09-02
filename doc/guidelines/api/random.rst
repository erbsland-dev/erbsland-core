****************************
Random Domain API Guidelines
****************************

Core Semantics
==============

.. code-block:: text

    pseudo-random = fast seeded sequence for non-security use
    secure = operating-system entropy suitable for security-sensitive values

Primary Types
=============

.. code-block:: text

    Random // common generator interface with selection, building, shuffling and byte-fill helpers
    FastRandom // reproducible pseudo-random generator when constructed with an explicit seed
    SecureRandom // cryptographic generator backed by the platform entropy source

Secondary Types
===============

.. code-block:: text

    ThreadSafeFastRandom // synchronized FastRandom wrapper for shared generator instances
    RandomError // failure to obtain secure system entropy

Source Patterns
===============

.. code-block:: text

    o.getInt❮width❯(minimum, maximum) -> int❮width❯_t // random signed integer in inclusive range
    o.getUInt❮width❯(minimum, maximum) -> uint❮width❯_t // random unsigned integer in inclusive range
    o.getDouble(minimum, maximum) -> double // random floating-point value in ordered range
    o.getBool() -> bool // random boolean
    o.isSecure() -> bool // report security suitability and automatic marking of owning results
    o.fillBytes(destination) -> void // fill an existing byte span with random bytes

Selection Patterns
==================

.. code-block:: text

    o.selectInteger(minimum, maximum) -> T // random native integer in inclusive ordered range
    o.selectInteger(range) -> T // random native integer from an IntegerRange
    o.selectIndex(count) -> unit::ItemIndex // random index in [0, count)
    o.selectElement(choices[, valueIfEmpty]) -> T // random element or fallback for empty choices

Build Patterns
==============

.. code-block:: text

    o.buildIntegerList(count, minimum, maximum) -> util::List<T> // random integers with replacement
    o.buildString(length, characters) -> text::String // random UTF-8 text; secure generators mark its allocation
    o.buildByteBlock(length) -> mem::ByteBlock // random bytes; secure generators mark storage before filling
    o.buildByteBuffer(length) -> mem::ByteBuffer // random bytes; secure generators enable sensitive mode before filling
    o.buildElementList(count, choices) -> util::List<T> // sample elements with replacement
    o.buildUniqueElementList(count, choices) -> util::List<T> // sample elements without replacement

Mutation Patterns
=================

.. code-block:: text

    o.shuffle(span/vector/list) -> void // randomly reorder values in place

Construction Patterns
=====================

.. code-block:: text

    T() // create an automatically seeded pseudo-random generator
    T(seed) // create a reproducible pseudo-random sequence
    o.isSecure() -> bool // report security suitability and sensitive owning-result construction
    o.❮draw❯(...) // report unavailable system entropy with RandomError

Application Patterns
====================

.. code-block:: text

    o.random() -> ThreadSafeFastRandom& // access the shared application pseudo-random generator
    o.secureRandom() -> SecureRandom& // access the shared application secure generator
