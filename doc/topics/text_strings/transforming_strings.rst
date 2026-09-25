..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    !single: Transforming Strings
    single: Immutable String Transformations
    single: Copy-returning String Operations

********************
Transforming Strings
********************

Most text changes are easier to reason about when the original value stays intact.
You start with a :cpp:type:`String <erbsland::text::String>`, ask for the value you want, and give that result a name.
The source remains available for another decision, a diagnostic, or a different transformation.

Erbsland Core expresses this style through past-tense operations such as ``trimmed()``, ``replacedAll()``,
``removed()``, and ``normalized()``.
This page shows how these copy-returning operations fit together, what they cost, and when a sequence of changes has
become a true editing workflow that belongs in ``StringEditor``.

A New Value or One Mutable Working Copy?
========================================

A single transformation is naturally expressed as a returned ``String``.
Several independent derived values should also remain ``String`` operations because each result has clear value
semantics.

Use :cpp:type:`StringEditor <erbsland::text::StringEditor>` when the algorithm is genuinely a sequence of dependent
mutations to one working value.
That avoids creating a succession of intermediate results and can reuse unique capacity between steps.
See :doc:`/topics/text_strings/editing_strings_in_place` for that workflow.

Where New Character Storage Is Needed
=====================================

A slice selects a range and shares the original backing store, so it does not copy the selected character data.
A transformation that changes character data generally needs result storage and copies or encodes the resulting text.
Some no-op cases can return shared data, but code should select an operation for clarity rather than depend on that
optimization.

Repeatedly assigning transformed results inside a long edit chain can allocate several intermediate buffers.
If all steps form one mutation workflow, an editor is the more direct representation.
If the goal is construction from fragments, use the allocation-aware choices in
:doc:`/topics/text_formatting/building_strings` instead.

Give Each Meaningful Result a Name
==================================

Keep the original as ``String`` and name meaningful derived results.
This is particularly effective for canonicalization pipelines where each stage has a distinct purpose.

.. erbsland-demo::
    :source: text/String/CanonicalStationName.cpp
    :exec: text/string --demo CanonicalStationName
    :source-sha256: aff44463e8f64f80450348a4254009fd84d203290f1f3394abcc7cc5e3f51be7

.. code-block:: cpp

    /// `String::transformed()` can create a canonical text form with a single character-mapping function.
    ///
    /// ASCII-only mappings are useful for machine-readable identifiers because they leave non-ASCII characters untouched
    /// and avoid the Unicode database.
    void canonicalStationName() {
        const auto displayName = el::String{"Module ORBITE-Äther 07"_el};
        auto canonicalName = displayName.transformed(el::Char::toAsciiLowercase);

        el::io::printLine("Display name ..: "_el, displayName);
        el::io::printLine("Canonical .....: "_el, canonicalName);
    }

.. erbsland-ansi::
    :escape-char: ␛

    Display name ..: Module ORBITE-Äther 07
    Canonical .....: module orbite-Äther 07

.. erbsland-demo-end::

Replace or Remove Without Losing the Source
===========================================

Use byte indexes when positions came from UTF-8 search operations; those indexes already address native storage.
Use code-point coordinates when a short, fixed-shape value is naturally described in decoded characters.

``replaced()``, ``replacedFirst()``, and ``replacedAll()`` return changed values.
``removed()``, ``removedFirst()``, and ``removedAll()`` return values without the selected range, text, or character
set.
For a shared selection without character-data copying, use ``slice()`` instead of removal.

.. erbsland-demo::
    :source: text/String/CharacterReplacement.cpp
    :exec: text/string --demo CharacterReplacement
    :source-sha256: fd3a657e42c5ab61fd2335a979d9630eac58843f51a8b19e4dc02dd47dae48c1

.. code-block:: cpp

    /// A `String` allows building a new string with text or characters replaced.
    /// The replacement happens in one pass, without creating intermediate copies.
    /// Also, if no text is replaced, the original string is returned.
    void characterReplacement() {
        const auto note = el::String{"⛈️ Gewitter; 🍃 Wind; 🌫 Nebel; ☔️ Regen; 🍃 Wind"_el};
        const auto semicolon = el::CharSet{U';'};
        auto emojisAndSpace = el::CharSet::from(el::UnicodeCategory::OtherSymbol);
        emojisAndSpace.add(U' ');

        // Replace all semicolons with a comma.
        el::io::printLine("Original ....: \""_el, note, "\""_el);
        auto withComma = note.replacedAll(semicolon, U',');
        el::io::printLine("Compact .....: \""_el, withComma, "\""_el);

        // Replace the semicolon + space sequence with a longer ` → ` sequence.
        auto withArrow = note.replacedAll("; "_el, " → "_el);
        el::io::printLine("Arrows ......: \""_el, withArrow, "\""_el);

        // Remove the emojis and space from the text.
        auto plainCompact = note.replacedAll(emojisAndSpace, el::String{});
        el::io::printLine("Plain .......: \""_el, plainCompact, "\""_el);

        // Add a bit of sunshine by replacing all `🍃 Wind` with `☀️ Sun`
        auto sunnySpots = note.replacedAll("🍃 Wind"_el, "☀️ Sun"_el);
        el::io::printLine("Sunny .......: \""_el, sunnySpots, "\""_el);

        // When no text gets replaced, the original string is returned
        el::io::printLine("\nNo change, no copy:");
        el::io::printLine("Original ....: \""_el, note, "\""_el);

        el::io::printLine("Sunny .......: \""_el, sunnySpots, "\""_el);
    }

.. erbsland-ansi::
    :escape-char: ␛

    Original ....: "⛈️ Gewitter; 🍃 Wind; 🌫 Nebel; ☔️ Regen; 🍃 Wind"
    Compact .....: "⛈️ Gewitter, 🍃 Wind, 🌫 Nebel, ☔️ Regen, 🍃 Wind"
    Arrows ......: "⛈️ Gewitter → 🍃 Wind → 🌫 Nebel → ☔️ Regen → 🍃 Wind"
    Plain .......: "️Gewitter;Wind;Nebel;️Regen;Wind"
    Sunny .......: "⛈️ Gewitter; ☀️ Sun; 🌫 Nebel; ☔️ Regen; ☀️ Sun"

    No change, no copy:
    Original ....: "⛈️ Gewitter; 🍃 Wind; 🌫 Nebel; ☔️ Regen; 🍃 Wind"
    Sunny .......: "⛈️ Gewitter; ☀️ Sun; 🌫 Nebel; ☔️ Regen; ☀️ Sun"

.. erbsland-demo-end::

Changes That Depend on Unicode Text
===================================

Case conversion and Unicode normalization may change both the number of code points and the native encoded length.
They therefore produce new values rather than exposing writable storage.
Read :doc:`/topics/text_strings/normalizing_strings` before comparing or storing canonically equivalent text.

Trimming and character-set removal are useful for cleanup at boundaries.
When a returned range can represent the result, the implementation can retain shared storage instead of copying the kept
characters.

.. erbsland-demo::
    :source: text/String/TrimCharacterSet.cpp
    :exec: text/string --demo TrimCharacterSet
    :source-sha256: cfe48e302d5b50c675dbea438f72e1a8e46de71dadc334cb20cdfe26456b3de2

.. code-block:: cpp

    /// `String::trimmed()` returns a view with selected characters removed from the front, back, or both sides.
    ///
    /// With a custom `CharSet`, trimming is not limited to whitespace.
    void trimCharacterSet() {
        const auto raw = el::String{"*** signal-orbite ;; "_el};
        static const auto border = el::CharSet{" *;"_el};

        auto clean = raw.trimmed(border);
        auto frontOnly = raw.trimmed(border, el::StringSide::Front);
        auto backOnly = raw.trimmed(border, el::StringSide::Back);

        el::io::printLine("Raw .......: \"", raw, "\""_el);
        el::io::printLine("Both sides : \"", clean, "\""_el);
        el::io::printLine("Front only : \"", frontOnly, "\""_el);
        el::io::printLine("Back only .: \"", backOnly, "\""_el);
    }

.. erbsland-ansi::
    :escape-char: ␛

    Raw .......: "*** signal-orbite ;; "
    Both sides : "signal-orbite"
    Front only : "signal-orbite ;; "
    Back only .: "*** signal-orbite"

.. erbsland-demo-end::
