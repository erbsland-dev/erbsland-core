..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    !single: Shuffling and Selecting Elements
    single: selectElement
    single: selectIndex
    single: buildElementList
    single: buildUniqueElementList
    single: shuffle
    single: Sampling with Replacement
    single: Sampling without Replacement
    single: Selecting One Element
    single: Selecting an Index
    single: Element Sampling Demo
    single: Empty Inputs
    single: Shuffling

.. _random-shuffling-and-selecting:

********************************
Shuffling and Selecting Elements
********************************

This page shows how to select, sample, and shuffle elements with the random API.
You will learn when to select an element directly, when to select an index, how sampling with and without replacement
differs, and how to shuffle mutable sequences in place.

Random element helpers keep code focused on the domain choice instead of on manual index math.
They support Erbsland containers, standard vectors, spans, and initializer lists.
Use the higher-level helpers when the random choice itself is the important operation.
Use :cpp:func:`selectIndex() <erbsland::random::Random::selectIndex>` only when the index is part of the surrounding
algorithm.

Element Sampling Demo
=====================

The following demo uses a color-palette workflow to show the complete element API.
It demonstrates single selection, empty-input fallbacks, index selection, sampling with replacement, sampling without
replacement, and in-place shuffling.

.. erbsland-demo::
    :source: random/RandomTopics/ElementSampling.cpp
    :exec: random/random_topics --demo ElementSampling
    :source-sha256: 880fabe7eabc41c3748bb38d4081318bbd3998d0d140c956bdbb66f49a2d4196

.. code-block:: cpp

    /// Element helpers select, sample, and shuffle values without manual index math.
    ///
    /// Use `selectElement()` for a single choice, `buildElementList()` for sampling
    /// with replacement, `buildUniqueElementList()` for sampling without
    /// replacement, and `shuffle()` when the complete order should change in place.
    void elementSampling() {
        auto &random = el::application().random();
        auto palette = el::StringList{
            "błękit"_el,
            "zieleń"_el,
            "karmin"_el,
            "złoto"_el,
        };

        // Select one element directly or provide a fallback for empty input.
        const auto accent = random.selectElement(palette);
        const auto fallback = random.selectElement(el::StringList{}, el::String{"biel"_el});
        el::io::printLine("Accent color : "_el, accent);
        el::io::printLine("Fallback     : "_el, fallback);

        // Select an index when code needs to update a separate structure.
        const auto index = random.selectIndex(palette.count());
        el::io::printLine("Accent index : "_el, index.toSizeT());

        // Build repeated and unique samples from the same choices.
        const auto gradient = random.buildElementList(el::ItemCount{5U}, palette);
        const auto studySet = random.buildUniqueElementList(el::ItemCount{3U}, palette);
        el::io::printLine("Gradient     : "_el, gradient.join(", "_el));
        el::io::printLine("Study set    : "_el, studySet.join(", "_el));

        // Shuffle rearranges all elements while preserving the original values.
        random.shuffle(palette);
        el::io::printLine("Shuffled     : "_el, palette.join(", "_el));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Accent color : złoto
    Fallback     : biel
    Accent index : 1
    Gradient     : błękit, zieleń, karmin, złoto, karmin
    Study set    : karmin, zieleń, złoto
    Shuffled     : błękit, złoto, karmin, zieleń

.. erbsland-demo-end::

Selecting One Element
=====================

Use :cpp:func:`selectElement() <erbsland::random::Random::selectElement>` when you want one value from a non-empty set
of choices.
The method returns the selected element directly, so user code can express the domain decision without count checks,
index conversion, or manual access into the container.

This is usually more readable than selecting an index first.
For example, selecting a terrain type, a color, a test case, or a simulated state is often the real operation.
The index is only an implementation detail in those cases.

Empty Inputs
============

When an empty input is possible, pass an explicit fallback value as the second argument to
:cpp:func:`selectElement() <erbsland::random::Random::selectElement>`.
The fallback keeps the empty case visible at the call site while still avoiding a separate branch when a default value
is acceptable.

This style works well for optional configuration, user-provided lists, or generated data where an empty list has a
reasonable default meaning.
When an empty list is an error in your domain, validate it before calling the random helper instead of hiding the
problem behind a fallback.

Selecting an Index
==================

Use :cpp:func:`selectIndex() <erbsland::random::Random::selectIndex>` when the random index is part of the surrounding
algorithm.
This is useful when you need to update a second data structure, mark a selected slot, replace an element in place, or
keep the index for later processing.

For zero or infinite counts, :cpp:func:`selectIndex() <erbsland::random::Random::selectIndex>` returns
``ItemIndex::noIndex()``.
Check ``ItemIndex::isValid()`` or ``ItemIndex::isNoIndex()`` before using an index that came from a dynamic count.

Sampling with Replacement
=========================

Use :cpp:func:`buildElementList() <erbsland::random::Random::buildElementList>` when you need several independent
choices from the same input.
The method samples with replacement, so the same source element may appear more than once.

This matches situations such as dice rolls, repeated measurements, weather states, randomized test samples, or
procedural content where each draw should be independent from the previous one.
A repeated value is not a collision in this model.
It is part of the intended result.

If the requested count is zero or infinite, or if the choices are empty, the result is an empty list.

Sampling without Replacement
============================

Use :cpp:func:`buildUniqueElementList() <erbsland::random::Random::buildUniqueElementList>` when each source element may
appear at most once in the result.
The method samples without replacement, so the result never contains more elements than the input choices.

This is useful for drawing a study set, assigning unique roles, selecting distinct test cases, or choosing a subset of
available options.
If you request more elements than the input contains, the result is capped to the available choices.

The relative order of the result is random.
Conceptually, the choices are shuffled first and the requested prefix is returned.

Shuffling
=========

Use :cpp:func:`shuffle() <erbsland::random::Random::shuffle>` when the complete order should change in place.
The operation preserves all elements and changes only their order.

Shuffling is useful for randomizing a deck, reordering test cases, mixing generated content, or presenting known values
in a fresh order.
Because the operation modifies the container, use it when the new order is the desired result.

:cpp:func:`shuffle() <erbsland::random::Random::shuffle>` works with mutable lists, vectors, and spans.
For ordered sets and hash sets, convert to a list first because sets do not have a mutable positional order.
