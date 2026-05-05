..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    !single: Editing Strings
    single: String Editing
    single: String Modifiers
    single: clear
    single: reset
    single: append
    single: remove
    single: keep
    single: replace
    single: truncate
    single: aligned

***************
Editing Strings
***************

:cpp:type:`String <erbsland::text::String>` is the editable string type for common application code.
Use it when text must change after it was created: fields are inserted, ranges are removed, markers are replaced, or a
label is padded or shortened for display.

The examples on this page use the common UTF-8 :cpp:type:`String <erbsland::text::String>` alias.
The same editing model is available for the width-specific :cpp:class:`U8String <erbsland::text::U8String>`,
:cpp:class:`U16String <erbsland::text::U16String>`, and
:cpp:class:`U32String <erbsland::text::U32String>` types.

You will learn how to choose between in-place and copy-based operations, when byte ranges are the most efficient
coordinate system, and how to use higher-level editing helpers for replacement, alignment, and truncation.

Choosing an Editing Operation
=============================

Most editing operations have two forms.
The in-place form changes the string and returns the same string object when chaining is useful.
The copy form has a past-tense name and returns the edited result while leaving the source unchanged.

For example, removal is available as :cpp:func:`remove() <erbsland::text::U8String::remove>` and
:cpp:func:`removed() <erbsland::text::U8String::removed>`, replacement as
:cpp:func:`replace() <erbsland::text::U8String::replace>` and
:cpp:func:`replaced() <erbsland::text::U8String::replaced>`, and truncation as
:cpp:func:`truncate() <erbsland::text::U8String::truncate>` and
:cpp:func:`truncated() <erbsland::text::U8String::truncated>`.

Use byte ranges when indexes came from string search functions.
For UTF-8 text, this keeps positions in the native storage coordinate system and avoids unnecessary conversions.
Use code-point ranges for short, fixed-shape text where the edit position is naturally counted as decoded characters.

For creating a new result from many fragments, prefer :cpp:class:`StringBuilder <erbsland::text::StringBuilder>`.
Direct appending on :cpp:type:`String <erbsland::text::String>` is best when the string already exists and only a small
number of edits is needed.

Clearing, Resetting, and Appending
==================================

:cpp:func:`clear() <erbsland::text::U8String::clear>` removes all characters but keeps allocated storage for reuse.
:cpp:func:`reset() <erbsland::text::U8String::reset>` returns the string to its default empty state and releases the
reserved storage.

This distinction matters in parsers, formatters, and reusable buffers.
Clear a temporary string when the next edit pass will likely need a similar capacity.
Reset it when the storage should be released or the string should behave like a freshly constructed object.

:cpp:func:`append() <erbsland::text::U8String::append>` can append text or repeat a single Unicode code point.
It is convenient for small edits on an already editable string.

.. erbsland-demo::
    :source: text/String/ClearResetAndAppend.cpp
    :exec: string --demo ClearResetAndAppend
    :source-sha256: b67d9e5fab0e8822bd34a57e109691e007dabc4552a1327af82fa339286da763

.. code-block:: cpp

    /// `clear()` removes the text while keeping the allocated storage available for
    /// reuse. `reset()` returns the string to its initial empty state and releases
    /// the reserved storage.
    ///
    /// Use `append()` for a small number of direct edits on an existing string. When
    /// a result is produced from many fragments in a loop, prefer `StringBuilder`
    /// because it is designed for incremental construction.
    void clearResetAndAppend() {
        auto draft = "ridge log"_els;
        draft.reserve(el::ByteLength{80U});

        // Append text and repeated code points directly to the editable string.
        draft.append(": "_el).append("lichen"_el).append(", "_el).append("moss"_el);
        draft.append(U'·', el::CpLength{3U});
        printDraftState("Draft"_el, draft);

        // Clear keeps the reserved storage for the next edit pass.
        draft.clear();
        draft.append("reused after clear"_el);
        printDraftState("After clear"_el, draft);

        // Reset releases the storage and starts from the default empty state.
        draft.reset();
        draft.append("fresh after reset"_el);
        printDraftState("After reset"_el, draft);
    }

.. erbsland-ansi::
    :escape-char: ␛

    Draft: ridge log: lichen, moss···
      bytes=29 capacity=80
    After clear: reused after clear
      bytes=18 capacity=80
    After reset: fresh after reset
      bytes=17 capacity=19

.. erbsland-demo-end::

Removing and Keeping Ranges
===========================

:cpp:func:`remove() <erbsland::text::U8String::remove>` deletes a byte or code-point range.
:cpp:func:`keep() <erbsland::text::U8String::keep>` does the inverse and keeps only the selected range.
The copy forms are :cpp:func:`removed() <erbsland::text::U8String::removed>` and
:cpp:func:`kept() <erbsland::text::U8String::kept>`.

When a range is found by searching, keep it in byte coordinates and pass it directly to
:cpp:class:`ByteRange <erbsland::unit::IntegerUnitRange>`.
For a small label or token with a known shape, :cpp:class:`CpRange <erbsland::unit::IntegerUnitRange>` can make the
intent clearer.

.. erbsland-demo::
    :source: text/String/RemoveAndKeepRanges.cpp
    :exec: string --demo RemoveAndKeepRanges
    :source-sha256: 4d11190346b8700f6a6c56959e072b0e190cad203bf470240e2595566255d256

.. code-block:: cpp

    /// Range-based editing works best with byte indexes returned by string search
    /// functions. Those indexes already point into the native UTF-8 storage and can
    /// be passed directly to `ByteRange`.
    ///
    /// Code-point ranges are useful for short, fixed-shape labels where positions
    /// are naturally counted as decoded characters. For long UTF-8 text, byte
    /// ranges avoid repeated scans from the beginning of the string.
    void removeAndKeepRanges() {
        auto record = "site=Åsen|weather=klart|note=sol"_els;

        // Search returns byte indexes, so the field can be removed without conversion.
        const auto weatherStart = record.find("weather="_el);
        const auto noteStart = record.find("note="_el);
        auto compactRecord = record;
        compactRecord.remove(el::ByteRange{weatherStart, noteStart});

        // Keep only the field value by reusing byte indexes from the same search path.
        const auto siteValueStart = record.find("="_el) + "="_el.length();
        const auto siteValueEnd = record.find("|"_el, siteValueStart);
        auto siteName = record;
        siteName.keep(el::ByteRange{siteValueStart, siteValueEnd});

        // Code-point ranges are readable for small labels with fixed structure.
        auto label = "🌙Luna-04"_els;
        label.keep(el::CpRange{el::CpIndex{1U}, el::CpLength{4U}});

        el::io::printLine("Original: "_el, record);
        el::io::printLine("After remove: "_el, compactRecord);
        el::io::printLine("Kept site: "_el, siteName);
        el::io::printLine("Kept label text: "_el, label);
    }

.. erbsland-ansi::
    :escape-char: ␛

    Original: site=Åsen|weather=klart|note=sol
    After remove: site=Åsen|note=sol
    Kept site: Åsen
    Kept label text: Luna

.. erbsland-demo-end::

Removing Text and Characters
============================

:cpp:func:`removeFirst() <erbsland::text::U8String::removeFirst>` removes only the first matching text occurrence.
:cpp:func:`removeAll() <erbsland::text::U8String::removeAll>` removes every matching occurrence.

There are two useful all-removal forms.
Pass a :cpp:class:`CharSet <erbsland::text::CharSet>` when every matching character should disappear.
Pass a string view when a complete text fragment should be removed.
Text removal works on decoded text, and an optional comparison function can be used for case-folded matching.

.. erbsland-demo::
    :source: text/String/RemoveFirstAndAll.cpp
    :exec: string --demo RemoveFirstAndAll
    :source-sha256: 4635fdab710ede0cb9cdde8cc0765d05650bd69ddae2e813e84ee59333ace1f7

.. code-block:: cpp

    /// `removeFirst()` changes only the first matching text occurrence.
    /// `removeAll()` removes every matching text occurrence or every character from
    /// a character set.
    ///
    /// Text matching is decoded Unicode text matching. You can pass a comparison
    /// function such as `Char::compareCaseFolded` when case-insensitive matching is
    /// required.
    void removeFirstAndAll() {
        const auto source = "mist :: frost :: mist :: aurora"_els;

        auto firstOnly = source;
        firstOnly.removeFirst("mist"_el);

        auto allMist = source;
        allMist.removeAll("mist"_el);

        auto withoutSeparators = source;
        withoutSeparators.removeAll(el::CharSet{": "_el});

        auto folded = "Ähre | äHRE | aster"_els;
        folded.removeAll("ähre"_el, el::Char::compareCaseFolded);

        el::io::printLine("Source: "_el, source);
        el::io::printLine("First text removed: "_el, firstOnly);
        el::io::printLine("All text removed: "_el, allMist);
        el::io::printLine("Characters removed: "_el, withoutSeparators);
        el::io::printLine("Case-folded removal: "_el, folded);
    }

.. erbsland-ansi::
    :escape-char: ␛

    Source: mist :: frost :: mist :: aurora
    First text removed:  :: frost :: mist :: aurora
    All text removed:  :: frost ::  :: aurora
    Characters removed: mistfrostmistaurora
    Case-folded removal:  |  | aster

.. erbsland-demo-end::

Inserting and Replacing
=======================

:cpp:func:`insert() <erbsland::text::U8String::insert>` adds text at a byte or code-point index.
:cpp:func:`replace() <erbsland::text::U8String::replace>` replaces a selected range with new text.
:cpp:func:`replaceFirst() <erbsland::text::U8String::replaceFirst>` changes one matching text occurrence, while
:cpp:func:`replaceAll() <erbsland::text::U8String::replaceAll>` changes all matching text occurrences or all characters
from a character set.

The same coordinate rule applies here as for removal.
Search first, then edit with byte indexes.
Use code-point positions only when that is the natural way to describe the edit.

.. erbsland-demo::
    :source: text/String/InsertAndReplace.cpp
    :exec: string --demo InsertAndReplace
    :source-sha256: f3a34fa2bc5a4b20ca36d16107829c8200ea0f36666c7371105778dcc4e2d396

.. code-block:: cpp

    /// `insert()` adds text at a byte or code-point index. `replace()` changes a
    /// range, `replaceFirst()` changes the first matching text occurrence, and
    /// `replaceAll()` changes every matching text occurrence or character from a
    /// character set.
    ///
    /// Prefer byte indexes when they come from a search operation. Use code-point
    /// indexes and ranges when the text is short and the edit position is naturally
    /// counted in decoded characters.
    void insertAndReplace() {
        auto report = "Plot 07 | sky=grey | sky=grey"_els;

        // A code-point index is readable for inserting at the beginning.
        report.insert(el::CpIndex{0U}, "☀ "_el);

        // Search results are byte indexes and can be reused in a byte range.
        const auto plotNumber = report.find("07"_el);
        report.replace(el::ByteRange{plotNumber, plotNumber + "07"_el.length()}, "08"_el);

        // First and all variants make common text substitutions explicit.
        report.replaceFirst("grey"_el, "clear"_el);
        report.replaceAll("sky="_el, "himmel="_el);

        // A character set replacement handles multiple separators in one pass.
        report.replaceAll(el::CharSet{"|="_el}, U'·');

        auto token = "AβC"_els;
        token.replace(el::CpRange{el::CpIndex{1U}, el::CpLength{1U}}, "beta"_el);

        el::io::printLine("Edited report: "_el, report);
        el::io::printLine("Edited token: "_el, token);
    }

.. erbsland-ansi::
    :escape-char: ␛

    Edited report: ☀ Plot 08 · himmel·clear · himmel·grey
    Edited token: AbetaC

.. erbsland-demo-end::

Aligning and Truncating
=======================

:cpp:func:`aligned() <erbsland::text::U8String::aligned>` returns a padded copy with the requested decoded code-point
width.
It accepts a :cpp:class:`Alignment <erbsland::bgeo::Alignment>` value and an optional fill character.

:cpp:func:`truncate() <erbsland::text::U8String::truncate>` shortens a string in place.
:cpp:func:`truncated() <erbsland::text::U8String::truncated>` returns the shortened copy.
The :cpp:enum:`TruncateMode <erbsland::text::TruncateMode>` selects whether the beginning, middle, or end is removed.
An optional ellipsis can be inserted into the result.

.. erbsland-demo::
    :source: text/String/AlignAndTruncate.cpp
    :exec: string --demo AlignAndTruncate
    :source-sha256: a08e36e59f45ec63e270943c5d3857812c6e9523cce677377fc4681a399ce024

.. code-block:: cpp

    /// `aligned()` returns a padded copy of the string using decoded code-point
    /// length for the requested field width. This is useful for compact textual
    /// tables and labels.
    ///
    /// `truncate()` edits a string in place, while `truncated()` returns a shortened
    /// copy. Truncation works by decoded code-point length and can keep the
    /// beginning, middle, or end of the original text.
    void alignAndTruncate() {
        const auto label = "Alpenrose"_els;
        el::io::printLine("|"_el, label.aligned(el::CpLength{14U}, el::Alignment::Left, U'.'), "|"_el);
        el::io::printLine("|"_el, label.aligned(el::CpLength{14U}, el::Alignment::HCenter, U'.'), "|"_el);
        el::io::printLine("|"_el, label.aligned(el::CpLength{14U}, el::Alignment::Right, U'.'), "|"_el);

        const auto observation = "Observation: Alpenrose beside pale limestone under morning light"_els;
        el::io::printLine("End: "_el, observation.truncated(el::CpLength{28U}, el::TruncateMode::End, "..."_el));
        el::io::printLine("Middle: "_el, observation.truncated(el::CpLength{28U}, el::TruncateMode::Middle, "..."_el));
        el::io::printLine("Begin: "_el, observation.truncated(el::CpLength{28U}, el::TruncateMode::Begin, "..."_el));

        auto mutableObservation = observation;
        mutableObservation.truncate(el::CpLength{22U}, el::TruncateMode::End, "..."_el);
        el::io::printLine("In-place: "_el, mutableObservation);
    }

.. erbsland-ansi::
    :escape-char: ␛

    |Alpenrose.....|
    |..Alpenrose...|
    |.....Alpenrose|
    End: Observation: Alpenrose be...
    Middle: Observation: ...orning light
    Begin: ...stone under morning light
    In-place: Observation: Alpenr...

.. erbsland-demo-end::

A Complete Editing Pass
=======================

The following example combines the most common editing operations in one short workflow.
It starts with existing text, inserts a sentence, replaces repeated text, removes an unwanted range, appends a closing
sentence, and finally formats the result with line breaks.

This kind of workflow is a good fit for :cpp:type:`String <erbsland::text::String>` because the text already exists and
only a limited number of direct edits are required.
For larger generated documents, use :cpp:class:`StringBuilder <erbsland::text::StringBuilder>` instead.

.. erbsland-demo::
    :source: text/String/EditingText.cpp
    :exec: string --demo EditingText
    :source-sha256: 2b71e8a574dba07cbd2999b08dc18032f29256ae67769d22ad719af40e05c7d4

.. code-block:: cpp

    /// `String` is an owning, editable copy-on-write string type.
    /// Use it when you build text from scratch or modify existing text.
    /// Use `StringView` for parameters and stored read-only text.
    void editingText() {
        // Create an editable string from a string literal.
        auto story = el::String{"The frost lifts from the valley. A pale crocus opens beside the stone. "
                                "Der Wind trägt Blätter durch die Luft."_el};

        // Alternatively, create an editable string directly with the `""_els` literal.
        auto intro = "A short alpine field note:"_els;

        // Find the insertion position after the first sentence.
        auto firstFullStopIndex = story.findFirstOf({U'.'});
        story.advance(firstFullStopIndex, el::CpLength{2});

        // Insert a new sentence after the first sentence.
        story.insert(firstFullStopIndex, "Sunlight reaches the wet limestone. "_el);

        // Replace the main subject.
        story.replaceAll("pale crocus"_el, "violet gentian"_el);

        // Remove one sentence from the story.
        constexpr auto sentenceBeginText = "Der Wind"_el;
        auto sentenceBeginIndex = story.find(sentenceBeginText);
        constexpr auto sentenceEndText = "Luft."_el;
        auto sentenceEndIndex = story.find(sentenceEndText, sentenceBeginIndex) + sentenceEndText.length();
        story.remove({sentenceBeginIndex, sentenceEndIndex});

        // Append a final sentence.
        story.append(" 🌿 The day begins quietly."_el);

        // Add a line break after each mid-sentence.
        story.replaceAll(". "_el, ".\n"_el);

        el::io::printLine(intro);
        el::io::printLine(story);
    }

.. erbsland-ansi::
    :escape-char: ␛

    A short alpine field note:
    The frost lifts from the valley.
    Sunlight reaches the wet limestone.
    A violet gentian opens beside the stone.
     🌿 The day begins quietly.

.. erbsland-demo-end::