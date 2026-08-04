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
    :exec: random/random_topics --demo RandomValues
    :source-sha256: 957b5f6ee719e232c170afc4d673a542620a0de864322e05b3ff1647e4299d52

.. code-block:: cpp

    /// Random value helpers cover integers, floating-point values, booleans, bytes,
    /// and strings.
    ///
    /// Integer ranges are inclusive and can be passed either as two bounds or as an
    /// `IntegerRange`. Repeated draws, byte blocks, and random strings are built from
    /// the same shared application generator, so application code does not need to
    /// seed or manage its own engine for ordinary random values.
    void randomValues() {
        auto &random = el::application().random();

        // Draw individual integer values and reuse domain ranges.
        const auto chamber = random.getUInt32(1U, 6U);
        const auto chargeRange = el::IntegerRange<int>{12, 30};
        const auto charge = random.selectInteger(chargeRange);
        const auto pulseLevels = random.buildIntegerList(el::ItemCount{5U}, 1, 4);

        el::io::printLine("Artifact chamber   : "_el, chamber);
        el::io::printLine("Runic charge       : "_el, charge);
        el::io::printLine(
            "Pulses             : "_el, pulseLevels.count().toSizeT(), " draws, first="_el, pulseLevels.first());

        // Draw non-integer values from the same random source.
        const auto resonance = random.getDouble(0.25, 0.95);
        const auto isAwake = random.getBool();
        const auto resonanceText = el::StringFormat{"Resonance          : {:.2f}"_el}.build(resonance);
        el::io::printLine(resonanceText);
        el::io::printLine("Guardian awake     : "_el, el::BooleanFormat::yesNo(), isAwake);

        // Build byte and string values when the caller needs generated data.
        const auto bytes = random.buildByteBlock(el::ByteLength{4U});
        const auto alphabet = el::CharSet::fromPattern("A-Z0-9"_el);
        const auto label = random.buildString(el::CpLength{10U}, alphabet);
        el::io::printLine("First byte         : "_el, static_cast<unsigned>(bytes.get(el::ByteIndex{0U}).toUInt8()));
        el::io::printLine("Generated label    : "_el, label);
    }

.. erbsland-ansi::
    :escape-char: ␛

    Artifact chamber   : 4
    Runic charge       : 13
    Pulses             : 5 draws, first=1
    Resonance          : 0.54
    Guardian awake     : no
    First byte         : 153
    Generated label    : OIAUKOO2C5

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
``SecureRandom`` reports itself as secure, so :cpp:func:`buildByteBlock() <erbsland::random::Random::buildByteBlock>`
marks the allocation before generated bytes are written.

Random Strings
==============

Use :cpp:func:`buildString() <erbsland::random::Random::buildString>` when you need readable generated text from a
defined character set.
The method creates a UTF-8 string by selecting characters from a :cpp:class:`CharSet <erbsland::text::CharSet>`.

Choose a small explicit character set for identifiers that must fit into URLs, file names, command-line arguments, or
protocol fields.
For visible identifiers, choose characters that are accepted by the receiving system and easy for users to read.
``SecureRandom::buildString()`` returns a marked UTF-8 string for text secrets.
For binary secrets, use ``SecureRandom::buildByteBlock()`` so the returned allocation is automatically marked as
sensitive.

The requested length is measured in Unicode code points, not bytes.
This matters when the character set contains non-ASCII characters, because one visible character may require several
UTF-8 bytes.

If the requested length is zero or infinite, or if the character set is empty, the result is an empty string.
