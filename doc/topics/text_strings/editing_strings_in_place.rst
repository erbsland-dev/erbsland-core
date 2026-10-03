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

StringEditor Mutations and Index Validity
=========================================

Search functions return positions in the editor's current text.
Apply positions before an earlier mutation invalidates them, or search again after the mutation.
Byte indexes are the efficient choice when they originate from UTF-8 searches because no coordinate conversion is
required.

.. erbsland-demo::
    :source: text/StringEditor/EditingText.cpp
    :exec: text/string_editor --demo EditingText
    :sha256: 178420311fb30add2c049a89ac24668d3ca98ebf65bd88e5f9f87fadf080bc09

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
        auto firstFullStopIndex = story.findFirstOf(el::CharSet{U'.'});
        story.advance(firstFullStopIndex, el::CpCount{2});

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

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mA short alpine field note:
        The frost lifts from the valley.
        Sunlight reaches the wet limestone.
        A violet gentian opens beside the stone.
         🌿 The day begins quietly.␛[0m

.. erbsland-demo-end::

Removing and Keeping Text Ranges
================================

``remove()`` deletes the selected range from the editor.
``keep()`` discards everything outside the selected range.
Use ``keep()`` when the editor must remain mutable after narrowing; use ``String::slice()`` when a read-only shared view
is sufficient.

.. erbsland-demo::
    :source: text/StringEditor/RemoveAndKeepRanges.cpp
    :exec: text/string_editor --demo RemoveAndKeepRanges
    :sha256: 07756adadf4a44a8713f053b243116b95168c29abfb06e10f7174c82a98a6721

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
        label.keep(el::CpRange{el::CpIndex{1U}, el::CpCount{4U}});

        el::io::printLine("Original: "_el, record);
        el::io::printLine("After remove: "_el, compactRecord);
        el::io::printLine("Kept site: "_el, siteName);
        el::io::printLine("Kept label text: "_el, label);
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mOriginal: site=Åsen|weather=klart|note=sol
        After remove: site=Åsen|note=sol
        Kept site: Åsen
        Kept label text: Luna␛[0m

.. erbsland-demo-end::

Removing Matching Text and Character Sets
=========================================

``removeFirst()`` changes the first matching text occurrence.
``removeAll()`` removes every matching text occurrence or every character from a
:cpp:class:`CharSet <erbsland::text::CharSet>`.
An optional character comparison function supports operations such as case-folded matching.

.. erbsland-demo::
    :source: text/StringEditor/RemoveFirstAndAll.cpp
    :exec: text/string_editor --demo RemoveFirstAndAll
    :sha256: 782ccc093fdd24e41a1c553a2d4a532fc8c036f6c93ff2a693126115f53cf8df

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

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mSource: mist :: frost :: mist :: aurora
        First text removed:  :: frost :: mist :: aurora
        All text removed:  :: frost ::  :: aurora
        Characters removed: mistfrostmistaurora
        Case-folded removal:  |  | aster␛[0m

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
    :sha256: 441a5e16d6f98db3164df266964115265ba4e601ccdfa0815a4766e07e3ed4b6

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
        token.replace(el::CpRange{el::CpIndex{1U}, el::CpCount{1U}}, "beta"_el);

        el::io::printLine("Edited report: "_el, report);
        el::io::printLine("Edited token: "_el, token);
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mEdited report: ☀ Plot 08 · himmel·clear · himmel·grey
        Edited token: AbetaC␛[0m

.. erbsland-demo-end::

Truncating and Aligning Text by Code-Point Width
================================================

``truncate()`` shortens the editor to a decoded code-point width and can retain the beginning, middle, or end.
``align()`` pads the value to a requested code-point width.
Both mutate the working value; the corresponding past-tense ``String`` methods return a new read-only value.

.. erbsland-demo::
    :source: text/StringEditor/AlignAndTruncate.cpp
    :exec: text/string_editor --demo AlignAndTruncate
    :sha256: ad4b1cfcc995bfaa51839eaa4905b0183461e7498dd24e74732db93dcf1bccea

.. code-block:: cpp

    /// `aligned()` returns a padded read-only value, while `truncate()` shortens an
    /// editor in place. Both use decoded code-point lengths rather than UTF-8 bytes.
    void alignAndTruncate() {
        const auto label = el::String{"Alpenrose"_el};
        el::io::printLine("Aligned: |"_el, label.aligned(el::CpCount{14U}, el::Alignment::HCenter, U'.'), "|"_el);

        auto observation = el::StringEditor{"Observation: Alpenrose beside pale limestone under morning light"_el};
        observation.truncate(el::CpCount{28U}, el::TruncateMode::Middle, "..."_el);
        el::io::printLine("Truncated: "_el, observation);
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mAligned: |..Alpenrose...|
        Truncated: Observation: ...orning light␛[0m

.. erbsland-demo-end::

Reusing Editor Capacity with clear() and reset()
================================================

``clear()`` removes text while keeping capacity for another edit pass.
``reset()`` releases the editor state and returns it to a fresh empty value.
``append()`` is appropriate during editing and for small construction tasks, but repeated unreserved growth may
reallocate and copy the accumulated character data.

.. erbsland-demo::
    :source: text/StringEditor/ClearResetAndAppend.cpp
    :exec: text/string_editor --demo ClearResetAndAppend
    :sha256: f319c7116e011d9d5716425fcfc8a5a66e5718fca68b101c9926468e79801057

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
        draft.reserve(el::ByteCount{80U});

        // Append text and repeated code points directly to the editable string.
        draft.append(": "_el).append("lichen"_el).append(", "_el).append("moss"_el);
        draft.append(U'·', el::CpCount{3U});
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

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mDraft: ridge log: lichen, moss···
          bytes=29 capacity=80
        After clear: reused after clear
          bytes=18 capacity=80
        After reset: fresh after reset
          bytes=17 capacity=47␛[0m

.. erbsland-demo-end::

In-Place Unicode Normalization
==============================

``normalize()`` applies a Unicode normalization form to the editor in place.
Use it inside a wider mutation workflow; use ``String::normalized()`` when normalization is the only transformation and
the source should remain read-only.

.. erbsland-demo::
    :source: text/StringEditor/NormalizeText.cpp
    :exec: text/string_editor --demo NormalizeText
    :sha256: 9caebfcb2ecf9b86d1e5e7641f78b33759013aab39c31678aa253eed9301441a

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

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mBefore NFC: ガリウム
        After NFC : ガリウム␛[0m

.. erbsland-demo-end::

For capacity planning, copy-on-write behavior, and compaction, continue with
:doc:`/topics/text_strings/managing_string_editor_storage`.
