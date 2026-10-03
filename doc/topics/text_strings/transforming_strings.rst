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

Preparing text often means deriving a value while keeping the original available: a label with consistent spacing, a
machine-readable name, or a note with selected punctuation replaced.
The common helpers in ``<erbsland/StringTransformations.hpp>`` handle casing, word conversion, and whitespace cleanup.
String methods cover replacement, removal, trimming, and custom character mappings.

All these operations return a result without changing your input.
This page shows how to choose a transformation, combine a few steps, and recognize when the task belongs in an editor.

Common Transformations Return String Values
===========================================

A helper such as :cpp:func:`toLowercase() <erbsland::text::toLowercase>` takes a concrete Core string or editor and
returns a read-only string of the same encoding.
For ordinary application code, that means supplying a :cpp:type:`String <erbsland::text::String>` and receiving another
``String``.
Wrap a literal in ``String{...}`` when passing it directly; the helper templates do not accept literals or standard
strings on their own.

The source remains available for comparison or another derived value.
Passing a :cpp:type:`StringEditor <erbsland::text::StringEditor>` follows the same rule: the helper returns a string and
leaves the editor unchanged.
This makes the helpers a convenient first choice whenever their operation matches your task.

Unicode and ASCII Case Conversion
=================================

For display text containing accented letters or other scripts,
:cpp:func:`toLowercase() <erbsland::text::toLowercase>` and
:cpp:func:`toUppercase() <erbsland::text::toUppercase>` apply simple Unicode case mappings.
:cpp:func:`toCaseFolded() <erbsland::text::toCaseFolded>` produces a simple folded form for case-insensitive processing.
If you only need to compare two values, :doc:`comparing_strings` explains how to select a comparison policy directly.

An ASCII-based format has a narrower rule.
:cpp:func:`toAsciiLowercase() <erbsland::text::toAsciiLowercase>` and
:cpp:func:`toAsciiUppercase() <erbsland::text::toAsciiUppercase>` change only ASCII letters and leave characters such as
``Ä`` untouched.
Choose that behavior because it matches the format, rather than treating it as a substitute for Unicode casing.

.. erbsland-demo::
    :source: text/String/CaseTransformation.cpp
    :exec: text/string --demo CaseTransformation
    :sha256: 741f4d8ca961346f0940b6e890cc1840db7ffb319f7e1419c6cc3caae0bdd6c4

.. code-block:: cpp

    /// String transformation helpers return Unicode or ASCII case variants without changing the source.
    /// Simple Unicode mappings preserve the code-point count, while ASCII mappings leave non-ASCII letters unchanged.
    /// The original storage can be reused when the transformation does not change the text.
    void caseTransformation() {
        const auto title = el::String{"Forêt d'Été, Σκιερό Μονοπάτι"_el};

        // Derive case variants using simple Unicode mappings.
        el::io::printLine("Original ......: "_el, title);
        el::io::printLine("Lowercase .....: "_el, el::toLowercase(title));
        el::io::printLine("Uppercase .....: "_el, el::toUppercase(title));
        el::io::printLine("Case folded ...: "_el, el::toCaseFolded(title));

        // Using only ASCII methods can be faster and can avoid linking the Unicode database into the executable.
        const auto sensorName = el::String{"TEMP-ÄSTHETIK-07"_el};
        el::io::printLine("\nSensor name ...: "_el, sensorName);
        el::io::printLine("ASCII lower ...:  "_el, el::toAsciiLowercase(sensorName));
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mOriginal ......: Forêt d'Été, Σκιερό Μονοπάτι
        Lowercase .....: forêt d'été, σκιερό μονοπάτι
        Uppercase .....: FORÊT D'ÉTÉ, ΣΚΙΕΡΌ ΜΟΝΟΠΆΤΙ
        Case folded ...: forêt d'été, σκιερό μονοπάτι

        Sensor name ...: TEMP-ÄSTHETIK-07
        ASCII lower ...:  temp-Ästhetik-07␛[0m

.. erbsland-demo-end::

Simple Unicode case conversion maps each code point to one code point.
The encoded byte length can still change, and these helpers do not perform locale-dependent casing or multi-character
expansions.
Unicode normalization is a separate operation, explained in :doc:`normalizing_strings`.

For a label that needs an initial capital, :cpp:func:`toCapitalized() <erbsland::text::toCapitalized>` uppercases its
first code point and lowercases the rest: ``hELLO wORLD`` becomes ``Hello world``.
It treats the whole input as one word; it does not search past leading punctuation for a letter.
:cpp:func:`toCapitalization() <erbsland::text::toCapitalization>` selects lowercase, uppercase, or capitalized output
through :cpp:enum:`Capitalization <erbsland::text::Capitalization>` when the choice comes from a setting.
Both operations have corresponding ``toAscii...`` helpers for ASCII rules.

Cleaning Whitespace and Converting Word Boundaries
==================================================

A label received from a form or text file may contain tabs, repeated spaces, and padding.
:cpp:func:`toWhitespaceSimplified() <erbsland::text::toWhitespaceSimplified>` removes surrounding Unicode whitespace
and replaces each internal run with one space.
You can supply another separator when the destination needs it.
:cpp:func:`toAsciiWhitespaceSimplified() <erbsland::text::toAsciiWhitespaceSimplified>` applies the same operation to
ASCII whitespace only.

Once the words are prepared, :cpp:func:`toCamelCaseFromWords() <erbsland::text::toCamelCaseFromWords>` joins them into a
name.
By default it splits on Unicode whitespace, preserves the first word, and uppercases the first code point of later words
without changing their remaining letters.
An explicit capitalization applies to every word, including the first.
The expedition example requests ``Capitalized`` to obtain ``TemperaturaDelMare``:

.. erbsland-demo::
    :source: text/String/WordTransformations.cpp
    :exec: text/string --demo WordTransformations
    :sha256: e3922dedee38b987f258ada7b48114fa1ceb773759c55b2977b2b9959e70cb22

.. code-block:: cpp

    /// Prepare an expedition label for display, a type name, and a machine-readable key.
    /// Whitespace simplification keeps word boundaries; camel-case conversion joins or separates words.
    /// Explicit capitalization applies to each word, including the first.
    void wordTransformations() {
        // Clean spacing while keeping the original spelling of each word.
        const auto fieldLabel = el::String{"  temperatura\t del   mare  "_el};
        const auto label = el::toWhitespaceSimplified(fieldLabel);
        el::io::printLine("Label .......: "_el, label);

        // Derive names for a schema and its exported fields.
        const auto typeName = el::toCamelCaseFromWords(label, el::Capitalization::Capitalized);
        const auto heading = el::toWordsFromCamelCase(typeName);
        const auto fieldKey = el::toAsciiWordsFromCamelCase(typeName, U'_', el::Capitalization::Lowercase);
        el::io::printLine("Type name ...: "_el, typeName);
        el::io::printLine("Heading .....: "_el, heading);
        el::io::printLine("Field key ...: "_el, fieldKey);

        // Underscores and hyphens are separators only when the input format declares them.
        static const auto separators = el::CharSet{"_-"_el};
        const auto sensorKey = el::String{"sonda_CTD-mare"_el};
        el::io::printLine("Joined key ..: "_el, el::toCamelCaseFromWords(sensorKey, separators));
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mLabel .......: temperatura del mare
        Type name ...: TemperaturaDelMare
        Heading .....: Temperatura Del Mare
        Field key ...: temperatura_del_mare
        Joined key ..: sondaCTDMare␛[0m

.. erbsland-demo-end::

:cpp:func:`toWordsFromCamelCase() <erbsland::text::toWordsFromCamelCase>` separates an existing name into words.
It recognizes acronym boundaries, so ``HTTPServer`` becomes ``HTTP Server``.
The optional separator and capitalization let you derive a field key such as ``temperatura_del_mare`` from the same
name.
The ``toAscii...`` variants use ASCII letters and digits when identifying those boundaries and changing case.

Underscores and hyphens are ordinary characters unless you include them in an explicit separator
:cpp:class:`CharSet <erbsland::text::CharSet>`.
The final example declares both separators and preserves the acronym: ``sonda_CTD-mare`` becomes ``sondaCTDMare``.
These conversions arrange prepared words; validate the input separately if your destination restricts which characters
are allowed.
:doc:`working_with_character_sets` explains how to express that policy.

For the narrower convention of lowercasing ASCII letters and replacing ordinary spaces with underscores,
:cpp:func:`toIdentifierNormalized() <erbsland::text::toIdentifierNormalized>` provides one direct operation.
It leaves other characters unchanged, so the destination's identifier rules still apply.

Custom Character Mappings
=========================

An application-specific cleanup rule may need several decisions for each decoded character.
:cpp:func:`transformed() <erbsland::text::U8String::transformed>` expresses that rule as a mapping from one
:cpp:class:`Char <erbsland::text::Char>` to another.
The following field-note cleanup converts two fullwidth punctuation marks and removes a zero-width space while
preserving the remaining text.
It applies the same mapping to a read-only string and to a draft that will receive further edits:

.. erbsland-demo::
    :source: text/String/CustomCharacterMapping.cpp
    :exec: text/string --demo CustomCharacterMapping
    :sha256: 5e9b4463a71d6d5c4d57e98438abe82d30ff1a75d5007a2c1fc6907bc3cff878

.. code-block:: cpp

    /// Apply an application-specific mapping to decoded characters without changing the source string.
    /// Return the original character to keep it, a replacement to change it, or `Char::noCodePoint()` to omit it.
    /// The callback has no captures or observable side effects and does not throw.
    void customCharacterMapping() {
        const auto fieldNote = el::String{"stazione： Tir\u200Breno； boa： 07"_el};
        const auto mapCharacter = [](const el::Char character) noexcept -> el::Char {
            if (character == U'：') {
                return U':';
            }
            if (character == U'；') {
                return U',';
            }
            // Omit U+200B zero-width space.
            if (character == el::Char{0x200BU}) {
                return el::Char::noCodePoint();
            }
            return character;
        };
        const auto cleaned = fieldNote.transformed(mapCharacter);
        el::io::printLine("Field note: "_el, cleaned);

        // Assign the returned editor when this mapping is part of a mutable workflow.
        auto draft = el::StringEditor{fieldNote};
        draft = draft.transformed(mapCharacter);
        draft.append("; verificata"_el);
        el::io::printLine("Draft ....: "_el, draft);
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mField note: stazione: Tirreno, boa: 07
        Draft ....: stazione: Tirreno, boa: 07; verificata␛[0m

.. erbsland-demo-end::

Returning the input character keeps it; returning another character replaces it.
:cpp:func:`Char::noCodePoint() <erbsland::text::Char::noCodePoint>` omits the current character.
If a format defines a terminator after which no text belongs in the result,
:cpp:func:`Char::endOfData() <erbsland::text::Char::endOfData>` stops the mapping and excludes the terminator and the rest
of the input.

The callback is a :cpp:type:`TransformCharacterFn <erbsland::text::TransformCharacterFn>`: a ``noexcept`` function
pointer taking and returning ``Char``.
A captureless ``noexcept`` lambda, as above, fits that interface.
Keep the mapping free of observable side effects; the same character may be passed to it more than once.
A callback that counts visits or depends on neighboring characters does not meet that contract.
For stateful processing or output that expands one character into several, iterate with a reader or ``forEach()`` and
append to an editor, as shown in :doc:`character_access_and_parsing`.

On a ``String``, ``transformed()`` returns a ``String``.
On a ``StringEditor``, :cpp:func:`transformed() <erbsland::text::U8StringEditor::transformed>` returns another editor
and leaves the original unchanged.
Assigning that result back, as in ``draft = draft.transformed(mapCharacter)``, replaces the working value before the
next edit.
This is a returned-value operation, even when the receiver is mutable.
For untrusted text that must be well-formed, check its encoding before applying a tolerant decoded-character operation.
The validation sequence is covered in :doc:`working_with_character_sets`.

Replacing and Removing Text
===========================

When several characters all receive the same replacement, a character set is a simpler expression of that rule than a
callback.
:cpp:func:`replacedAll() <erbsland::text::U8String::replacedAll>` accepts either a set of individual characters or search
text that must match as a complete sequence.
In the weather note below, one result replaces semicolons with commas; another replaces each semicolon-and-space
sequence with an arrow.

.. erbsland-demo::
    :source: text/String/CharacterReplacement.cpp
    :exec: text/string --demo CharacterReplacement
    :sha256: c0023d94d165c02686e9162e9cab0cbf188a12de318d8b7df23981ad4629bbff

.. code-block:: cpp

    /// A `String` allows building a new string with text or characters replaced.
    /// A character set selects individual characters; search text selects complete sequences.
    /// Unchanged results share the source storage, including repeated identical replacements.
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
        auto plainCompact = note.removedAll(emojisAndSpace);
        el::io::printLine("Plain .......: \""_el, plainCompact, "\""_el);

        // Add a bit of sunshine by replacing all `🍃 Wind` with `☀️ Sun`
        auto sunnySpots = note.replacedAll("🍃 Wind"_el, "☀️ Sun"_el);
        el::io::printLine("Sunny .......: \""_el, sunnySpots, "\""_el);

        // Repeat the replacement after all wind entries have already been replaced.
        auto unchanged = sunnySpots.replacedAll("🍃 Wind"_el, "☀️ Sun"_el);
        el::io::printLine("\nNo change, no copy:"_el);
        el::io::printLine("Before ......: \""_el, sunnySpots, "\""_el);
        el::io::printLine("After .......: \""_el, unchanged, "\""_el);
        el::io::printLine("Same storage : "_el, unchanged.storageId() == sunnySpots.storageId());
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mOriginal ....: "⛈️ Gewitter; 🍃 Wind; 🌫 Nebel; ☔️ Regen; 🍃 Wind"
        Compact .....: "⛈️ Gewitter, 🍃 Wind, 🌫 Nebel, ☔️ Regen, 🍃 Wind"
        Arrows ......: "⛈️ Gewitter → 🍃 Wind → 🌫 Nebel → ☔️ Regen → 🍃 Wind"
        Plain .......: "️Gewitter;Wind;Nebel;️Regen;Wind"
        Sunny .......: "⛈️ Gewitter; ☀️ Sun; 🌫 Nebel; ☔️ Regen; ☀️ Sun"

        No change, no copy:
        Before ......: "⛈️ Gewitter; ☀️ Sun; 🌫 Nebel; ☔️ Regen; ☀️ Sun"
        After .......: "⛈️ Gewitter; ☀️ Sun; 🌫 Nebel; ☔️ Regen; ☀️ Sun"
        Same storage : true␛[0m

.. erbsland-demo-end::

:cpp:func:`replacedFirst() <erbsland::text::U8String::replacedFirst>` limits a change to the first match.
The corresponding :cpp:func:`removedFirst() <erbsland::text::U8String::removedFirst>` and
:cpp:func:`removedAll() <erbsland::text::U8String::removedAll>` omit matches.
Character sets operate on code points: removing an emoji's symbol code point can leave a variation selector behind, as
in the plain-text output above.
A visible symbol may contain several code points; choose complete search text when they need to be handled together.

For a known range, :cpp:func:`replaced() <erbsland::text::U8String::replaced>` and
:cpp:func:`removed() <erbsland::text::U8String::removed>` apply the change to that selection.
UTF-8 search positions are byte indexes, so they can describe byte ranges directly.
Code-point ranges are convenient when the selection comes from a character count.
See :doc:`finding_text_positions` for locating the text to change.

Trimming the Edges of a String
==============================

:cpp:func:`trimmed() <erbsland::text::U8String::trimmed>` removes ASCII whitespace from both ends by default and leaves
internal spacing alone.
A custom character set can include application-specific border markers, and
:cpp:enum:`StringSide <erbsland::text::StringSide>` limits trimming to the front or back:

.. erbsland-demo::
    :source: text/String/TrimCharacterSet.cpp
    :exec: text/string --demo TrimCharacterSet
    :sha256: 36a1a1b5bd7a0badbbe227ecf5b37f3d84dde78281a34ee5c42976c2476fa6f0

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

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mRaw .......: "*** signal-orbite ;; "
        Both sides : "signal-orbite"
        Front only : "signal-orbite ;; "
        Back only .: "*** signal-orbite"␛[0m

.. erbsland-demo-end::

Trimming returns an owning slice into the source storage.
For cleanup that also collapses whitespace between words, ``toWhitespaceSimplified()`` expresses that broader operation
directly.

Combining Transformations and Managing Storage
==============================================

A short sequence of derived values is often the clearest way to express a conversion, as in the label, type name, and
field key above.
The returned strings own their text, so you can retain any intermediate value you need.
An unchanged transformation can share the source storage; a changed result generally needs storage for its new text.
The repeated replacement in the weather-note example illustrates this sharing.

A slice keeps the source allocation alive even when it selects only a small range.
:cpp:func:`kept() <erbsland::text::U8String::kept>` creates a separate copy of a selected range when retaining the larger
allocation would be undesirable.
:doc:`slicing_and_splitting_strings` develops that lifetime choice.

When the task becomes a series of dependent insertions, removals, and replacements on one working value,
:cpp:type:`StringEditor <erbsland::text::StringEditor>` makes those mutations explicit and can reuse capacity.
Continue with :doc:`editing_strings_in_place` for that workflow, or
:doc:`/topics/text_formatting/building_strings` when you are assembling text from fragments.
