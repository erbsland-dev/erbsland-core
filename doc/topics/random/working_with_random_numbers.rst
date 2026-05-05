..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    !single: Working with Random Values
    single: Random
    single: FastRandom
    single: ThreadSafeFastRandom
    single: SecureRandom
    single: application
    single: IntegerRange
    single: getUInt32
    single: selectInteger
    single: buildIntegerList
    single: getDouble
    single: getBool
    single: fillBytes
    single: buildByteBlock
    single: buildString
    single: CharSet
    single: Integer Values
    single: Domain Ranges
    single: Lists of Integers
    single: Floating-Point Values
    single: Boolean Values
    single: Random Bytes
    single: Random Strings

.. _random-working-with-numbers:

**************************
Working with Random Values
**************************

This page shows the common value helpers provided by :cpp:class:`Random <erbsland::random::Random>`.
You will learn how to draw integers, floating-point values, booleans, byte blocks, and strings without manually drawing
raw bits and converting them yourself.

The same helpers are available on all concrete random generators.
This means you can use the same API with :cpp:class:`FastRandom <erbsland::random::FastRandom>`,
:cpp:class:`ThreadSafeFastRandom <erbsland::random::ThreadSafeFastRandom>`, and
:cpp:class:`SecureRandom <erbsland::random::SecureRandom>`.
Choose the generator for the required safety and reproducibility properties, then use the helper that best describes the
value you want to create.

Random Values Demo
==================

The following demo uses :cpp:func:`application() <erbsland::core::application>` to access the shared application
generator.
It demonstrates inclusive integer ranges, :cpp:class:`IntegerRange <erbsland::math::IntegerRange>`, repeated integer
draws, floating-point values, booleans, byte blocks, and random strings.

.. erbsland-demo::
    :source: random/RandomTopics/RandomValues.cpp
    :exec: random_topics --demo RandomValues

.. erbsland-demo-end::

Integer Values
==============

Use integer helpers when the result belongs to a discrete domain such as dice rolls, retry slots, map coordinates,
levels, counters, or randomly selected limits.

Integer ranges are inclusive.
Both bounds can be generated.
If you pass the bounds in reverse order, the generator treats them as the same range with ordered bounds.
This makes it safe to forward ranges from user input or configuration after you have validated the domain itself.

Use named methods such as :cpp:func:`getUInt32() <erbsland::random::Random::getUInt32>` when the integer width is
important.
This is useful for protocol fields, binary formats, hashes, masks, and other places where the exact integer type is part
of the surrounding API.

Use :cpp:func:`selectInteger() <erbsland::random::Random::selectInteger>` when the native C++ integer type already
expresses the domain value clearly.
For example, a small simulation setting may not need to expose whether the value was drawn through a 16-bit, 32-bit, or
native integer path.

Domain Ranges
=============

Use :cpp:class:`IntegerRange <erbsland::math::IntegerRange>` when a range is already part of your domain model.
This keeps the range definition close to the model and lets the random draw reuse it without unpacking the lower and
upper bound at each call site.

This style is especially useful when the same range is used for validation, configuration, display, and random
selection.
The range object becomes the single place that describes the allowed values.

Lists of Integers
=================

Use :cpp:func:`buildIntegerList() <erbsland::random::Random::buildIntegerList>` when you need several independent
integer draws.
The list is sampled with replacement, so values may repeat.

This is useful for compact test data, repeated measurements, randomized weights, procedural attributes, or any case
where every entry should be drawn independently.
If you need a shuffled list without repetition, build the list of allowed values first and shuffle it instead.

Passing a zero or infinite count returns an empty list.
This makes it safe to forward a count that was validated elsewhere without adding a special branch at every call site.

Floating-Point Values
=====================

Use :cpp:func:`getDouble(minimum, maximum) <erbsland::random::Random::getDouble>` for continuous values such as speeds,
weights, positions, probabilities, sensor samples, and simulation parameters.

The method returns a value in the selected interval using the generator's real-value distribution.
As with integer bounds, reversed minimum and maximum values are ordered automatically before drawing.

Format floating-point values explicitly in user-visible output when you need stable text.
For example, a generated value can be correct for the simulation while still needing a fixed number of decimal places in
logs, demos, or reports.

Boolean Values
==============

Use :cpp:func:`getBool() <erbsland::random::Random::getBool>` for an unbiased binary decision.
It returns ``true`` or ``false`` with equal probability.

This is useful for simulations, randomized tests, procedural content, and small choices where both outcomes should be
equally likely.
When one outcome should be more likely than the other, draw an integer or floating-point value and compare it with the
desired threshold.

Random Bytes
============

Use :cpp:func:`fillBytes() <erbsland::random::Random::fillBytes>` when you already own the destination memory.
Use :cpp:func:`buildByteBlock() <erbsland::random::Random::buildByteBlock>` when you want a new copy-on-write byte
block.

Byte helpers are useful for generated test data, binary identifiers, randomized buffers, and data that is later encoded
or written to a binary format.
For deterministic tests or non-security data generation, a seeded :cpp:class:`FastRandom <erbsland::random::FastRandom>`
can make the byte output repeatable.

Use :cpp:class:`SecureRandom <erbsland::random::SecureRandom>` for security-sensitive bytes.
This includes tokens, keys, nonces, salts, session identifiers, and any byte sequence where predictability would be a
problem.

Random Strings
==============

Use :cpp:func:`buildString() <erbsland::random::Random::buildString>` when you need readable generated text from a
defined character set.
The method creates a UTF-8 string by selecting characters from a :cpp:class:`CharSet <erbsland::text::CharSet>`.

Choose a small explicit character set for identifiers that must fit into URLs, file names, command-line arguments, or
protocol fields.
For visible identifiers, choose characters that are accepted by the receiving system and easy for users to read.
For binary secrets, prefer :cpp:func:`buildByteBlock() <erbsland::random::Random::buildByteBlock>` so every byte value
remains available.

The requested length is measured in Unicode code points, not bytes.
This matters when the character set contains non-ASCII characters, because one visible character may require several
UTF-8 bytes.

If the requested length is zero or infinite, or if the character set is empty, the result is an empty string.
