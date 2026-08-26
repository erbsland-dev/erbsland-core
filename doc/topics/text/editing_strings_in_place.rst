..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    !single: Editing Strings in Place
    single: StringEditor
    single: In-place Text Editing
    single: remove
    single: keep
    single: insert
    single: replace
    single: truncate

************************
Editing Strings in Place
************************

Sometimes a transformation is not one clean step.
You may need to locate a sentence, insert an annotation, replace a name, remove an obsolete field, and only then know
what the finished value should be.
Creating and naming an intermediate ``String`` for every one of those dependent changes would obscure the algorithm and
may allocate more temporary storage than necessary.

:cpp:type:`StringEditor <erbsland::text::StringEditor>` is the local mutable value for this kind of work.
This page follows one editor through range changes, replacements, truncation, reuse, and normalization, while also
showing where search positions and storage ownership need care.
Once the edit is complete, expose the result as :cpp:type:`String <erbsland::text::String>` and return to the ordinary
read-only model.

Follow One Value Through a Sequence of Edits
============================================

Search functions return positions in the editor's current text.
Apply positions before an earlier mutation invalidates them, or search again after the mutation.
Byte indexes are the efficient choice when they originate from UTF-8 searches because no coordinate conversion is
required.

.. erbsland-demo::
    :source: text/StringEditor/EditingText.cpp
    :exec: text/string_editor --demo EditingText
    :source-sha256: a930e4a87a66c07ce7de47ebbb62f4d2cc8e953c0261fb2fdd114f79ba35baf9

.. code-block:: cpp

    /// `StringEditor` is an owning, mutable working value for multi-step edits.
    /// Use `String` for parameters, stored read-only text, and ordinary
    /// copy-returning transformations.
    void editingText() {
        auto story = el::StringEditor{"The frost lifts from the valley. A pale crocus opens beside the stone. "
                                      "Der Wind trägt Blätter durch die Luft."_el};

        // Ordinary text values use the owning read-only string type.
        const auto intro = el::String{"A short alpine field note:"_el};

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

Narrow the Working Text by Range
================================

``remove()`` deletes the selected range from the editor.
``keep()`` discards everything outside the selected range.
Use ``keep()`` when the editor must remain mutable after narrowing; use ``String::slice()`` when a read-only shared view
is sufficient.

.. erbsland-demo::
    :source: text/StringEditor/RemoveAndKeepRanges.cpp
    :exec: text/string_editor --demo RemoveAndKeepRanges
    :source-sha256: 08fba8790fb976498d1963c57d714dadbf5e129fa55cef8083ad301338b08cca

.. code-block:: cpp

    /// Range-based in-place edits work best with byte indexes returned by search
    /// functions. Those indexes already point into the native UTF-8 storage and can
    /// be passed directly to `ByteRange`.
    ///
    /// Code-point ranges are useful for short, fixed-shape labels where positions
    /// are naturally counted as decoded characters. For long UTF-8 text, byte
    /// ranges avoid repeated scans from the beginning of the string.
    void removeAndKeepRanges() {
        const auto record = el::String{"site=Åsen|weather=klart|note=sol"_el};

        // Search returns byte indexes, so the field can be removed without conversion.
        const auto weatherStart = record.find("weather="_el);
        const auto noteStart = record.find("note="_el);
        auto compactRecord = el::StringEditor{record};
        compactRecord.remove(el::ByteRange{weatherStart, noteStart});

        // Keep only the field value by reusing byte indexes from the same search path.
        const auto siteValueStart = record.find("="_el) + "="_el.length();
        const auto siteValueEnd = record.find("|"_el, siteValueStart);
        auto siteName = el::StringEditor{record};
        siteName.keep(el::ByteRange{siteValueStart, siteValueEnd});

        // Code-point ranges are readable for small labels with fixed structure.
        auto label = el::StringEditor{"🌙Luna-04"_el};
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

Remove What the Text Contains
=============================

``removeFirst()`` changes the first matching text occurrence.
``removeAll()`` removes every matching text occurrence or every character from a
:cpp:class:`CharSet <erbsland::text::CharSet>`.
An optional character comparison function supports operations such as case-folded matching.

.. erbsland-demo::
    :source: text/StringEditor/RemoveFirstAndAll.cpp
    :exec: text/string_editor --demo RemoveFirstAndAll
    :source-sha256: fe2e79066f6d12ddfef2ab74e9e2b4ac032ed6a80fb1e9ab06a2beed7af74ab3

.. code-block:: cpp

    /// `removeFirst()` changes only the first matching text occurrence.
    /// `removeAll()` removes every matching occurrence or every character from
    /// a character set.
    ///
    /// Text matching is decoded Unicode text matching. You can pass a comparison
    /// function such as `Char::compareCaseFolded` when case-insensitive matching is
    /// required.
    void removeFirstAndAll() {
        const auto source = el::String{"mist :: frost :: mist :: aurora"_el};

        auto firstOnly = el::StringEditor{source};
        firstOnly.removeFirst("mist"_el);

        auto allMist = el::StringEditor{source};
        allMist.removeAll("mist"_el);

        auto withoutSeparators = el::StringEditor{source};
        withoutSeparators.removeAll(el::CharSet{": "_el});

        auto folded = el::StringEditor{"Ähre | äHRE | aster"_el};
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

Insert and Replace at Known Positions
=====================================

``insert()`` adds text at a byte or code-point index.
``replace()`` changes a selected range, while ``replaceFirst()`` and ``replaceAll()`` search and mutate in one
operation.
These operations may move following data inside the allocation; repeated large insertions near the beginning can
therefore be more expensive than appending.

.. erbsland-demo::
    :source: text/StringEditor/InsertAndReplace.cpp
    :exec: text/string_editor --demo InsertAndReplace
    :source-sha256: f69e4c84af30fc0cc81c444200f4f0a4923f6b54f451820dfece85745faef4ca

.. code-block:: cpp

    /// `insert()` adds text at a byte or code-point index. `replace()` changes a
    /// range, `replaceFirst()` changes the first matching text occurrence, and
    /// `replaceAll()` changes every matching occurrence in place.
    ///
    /// Prefer byte indexes when they come from a search operation. Use code-point
    /// indexes and ranges when the text is short and the edit position is naturally
    /// counted in decoded characters.
    void insertAndReplace() {
        auto report = el::StringEditor{"Plot 07 | sky=grey | sky=grey"_el};

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

        auto token = el::StringEditor{"AβC"_el};
        token.replace(el::CpRange{el::CpIndex{1U}, el::CpLength{1U}}, "beta"_el);

        el::io::printLine("Edited report: "_el, report);
        el::io::printLine("Edited token: "_el, token);
    }

.. erbsland-ansi::
    :escape-char: ␛

    Edited report: ☀ Plot 08 · himmel·clear · himmel·grey
    Edited token: AbetaC

.. erbsland-demo-end::

Fit Text into a Character Width
===============================

``truncate()`` shortens the editor to a decoded code-point width and can retain the beginning, middle, or end.
``align()`` pads the value to a requested code-point width.
Both mutate the working value; the corresponding past-tense ``String`` methods return a new read-only value.

.. erbsland-demo::
    :source: text/StringEditor/AlignAndTruncate.cpp
    :exec: text/string_editor --demo AlignAndTruncate
    :source-sha256: 7e423803c272716581e26aefb31db457d3707bcbb40718ee0a5cd32f4975008b

.. code-block:: cpp

    /// `aligned()` returns a padded read-only value, while `truncate()` shortens an
    /// editor in place. Both use decoded code-point lengths rather than UTF-8 bytes.
    void alignAndTruncate() {
        const auto label = el::String{"Alpenrose"_el};
        el::io::printLine("Aligned: |"_el, label.aligned(el::CpLength{14U}, el::Alignment::HCenter, U'.'), "|"_el);

        auto observation = el::StringEditor{"Observation: Alpenrose beside pale limestone under morning light"_el};
        observation.truncate(el::CpLength{28U}, el::TruncateMode::Middle, "..."_el);
        el::io::printLine("Truncated: "_el, observation);
    }

.. erbsland-ansi::
    :escape-char: ␛

    Aligned: |..Alpenrose...|
    Truncated: Observation: ...orning light

.. erbsland-demo-end::

Reuse an Editor for Another Pass
================================

``clear()`` removes text while keeping capacity for another edit pass.
``reset()`` releases the editor state and returns it to a fresh empty value.
``append()`` is appropriate during editing and for small construction tasks, but repeated unreserved growth may
reallocate and copy the accumulated character data.

.. erbsland-demo::
    :source: text/StringEditor/ClearResetAndAppend.cpp
    :exec: text/string_editor --demo ClearResetAndAppend
    :source-sha256: 956c77607bc7f40df1719483ced732bd3f94f849374e754e02bac7521343221e

.. code-block:: cpp

    /// `clear()` removes the text while keeping the allocated storage available for
    /// reuse. `reset()` returns the string to its initial empty state and releases
    /// the reserved storage.
    ///
    /// This pattern is useful when one local editor is reused for several editing
    /// passes. Reserve once before predictable growth; do not reserve before each
    /// append operation.
    void clearResetAndAppend() {
        auto draft = el::StringEditor{"ridge log"_el};
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

    void printDraftState(const el::String &label, const el::StringEditor &editor) {
        el::io::printLine(label, ": "_el, editor);
        el::io::printLine("  bytes="_el, editor.length().toSizeT(), " capacity="_el, editor.capacity().toSizeT());
    }

.. erbsland-ansi::
    :escape-char: ␛

    Draft: ridge log: lichen, moss···
      bytes=29 capacity=80
    After clear: reused after clear
      bytes=18 capacity=80
    After reset: fresh after reset
      bytes=17 capacity=47

.. erbsland-demo-end::

Normalize as Part of the Edit
=============================

``normalize()`` applies a Unicode normalization form to the editor in place.
Use it inside a wider mutation workflow; use ``String::normalized()`` when normalization is the only transformation and
the source should remain read-only.

.. erbsland-demo::
    :source: text/StringEditor/NormalizeText.cpp
    :exec: text/string_editor --demo NormalizeText
    :source-sha256: b7ef58bf0458c383cd8590fa14237b9ed411d4c1542511aeb4d6bd6e502da882

.. code-block:: cpp

    /// `normalize()` canonicalizes an editor in place. This decomposed Japanese
    /// element name uses a combining voiced mark, which NFC composes with the
    /// preceding katakana character.
    void normalizeText() {
        auto elementName = el::StringEditor{"カ\u3099リウム"_el};

        el::io::printLine("Before NFC: "_el, elementName);
        elementName.normalize(el::NormalizationForm::Nfc);
        el::io::printLine("After NFC : "_el, elementName);
    }

.. erbsland-ansi::
    :escape-char: ␛

    Before NFC: カリウム
    After NFC : ガリウム

.. erbsland-demo-end::

For capacity planning, copy-on-write behavior, and compaction, continue with
:doc:`managing_string_editor_storage`.
