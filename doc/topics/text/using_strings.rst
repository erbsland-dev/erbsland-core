..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    !single: Using Strings
    single: String Parameters
    single: String Storage
    single: Shared String Storage
    single: String Literals

*************
Using Strings
*************

:cpp:type:`String <erbsland::text::String>` is the type you can comfortably use at almost every ordinary text boundary.
It behaves like a read-only value, yet it owns or safely shares the storage that keeps its characters alive.
You can therefore pass a temporary literal, retain a slice, or copy a stored value without having to coordinate a
separate view lifetime.

This page develops that everyday model from the caller's perspective.
It explains how to accept text without needless overloads, how copies and returned values remain efficient, and when a
shared slice should be turned into independent storage.

One Parameter for Ordinary Read-only Text
=========================================

A function that only reads UTF-8 text does not need to know whether the caller has a literal, a stored value, or a local
editor.
A ``const String &`` parameter gives all three a common boundary: literal data can remain static, existing strings can
share their backing store, and an editor can expose its current value safely.
One signature is therefore enough without introducing a borrowed view lifetime or a family of convenience overloads.

.. erbsland-demo::
    :source: text/String/IdealFunctionParameter.cpp
    :exec: text/string --demo IdealFunctionParameter
    :source-sha256: 138caa39930fd6d48799410e9cb358a1bf15a86e7ee8a635a7cb58bd6d58b2b3

.. code-block:: cpp

    /// `String` is the preferred parameter type for functions that read text.
    ///
    /// It accepts common string inputs naturally:
    /// - string literals are used directly without copying,
    /// - existing strings share their storage,
    /// - local editors convert safely to read-only strings.
    ///
    /// From the caller's perspective, all variants behave the same.
    void idealFunctionParameter() {
        // A string literal can be passed directly.
        countEmojis("🌲🌲 Waldkonzert mit Fuchs 🦊 und Eule 🦉"_el);

        // An existing string can be passed without copying.
        const auto string = el::String{"Pluie douce sur les fleurs 🌧️🌷🌼"_el};
        countEmojis(string);

        // A short value assembled with an editor is accepted as read-only input.
        auto constructed = el::StringEditor{"Bosque nocturno: "_el};
        constructed.append("luna 🌙 y grillos 🦗"_el);
        countEmojis(constructed);
    }

    /// Count all emoji-like symbols in `text` and print the result.
    ///
    /// The function only needs read-only access to the text. It does not need to know
    /// whether the caller passed a literal, a string, or a short constructed value.
    void countEmojis(const el::String &text) {
        std::size_t emojiCount = 0;
        text.forEach([&](const el::Char character) mutable noexcept -> el::util::LoopStatus {
            if (character.isCategory(el::UnicodeCategory::OtherSymbol)) {
                ++emojiCount;
            }
            return el::LoopStatus::Continue;
        });

        el::io::printLine("Text ..........: \""_el, text, "\""_el);
        el::io::printLine("Emoji symbols .: ", emojiCount);
        el::io::printLine();
    }

.. erbsland-ansi::
    :escape-char: ␛

    Text ..........: "🌲🌲 Waldkonzert mit Fuchs 🦊 und Eule 🦉"
    Emoji symbols .: 4

    Text ..........: "Pluie douce sur les fleurs 🌧️🌷🌼"
    Emoji symbols .: 3

    Text ..........: "Bosque nocturno: luna 🌙 y grillos 🦗"
    Emoji symbols .: 2

.. erbsland-demo-end::

Values That Are Safe to Store and Return
========================================

Stored text is usually read far more often than it is changed, which makes ``String`` the honest member type.
Copying it normally copies a small handle and shares the backing store, while returning it by value benefits from normal
move and return-value optimization.

Keeping a ``StringEditor`` in a data structure merely because a future change is possible weakens that intent and makes
accidental mutation easier.
Create the editor at the moment a multi-step edit actually begins, and turn the completed result back into a ``String``
for long-lived storage.

.. erbsland-demo::
    :source: text/String/ConstructionAndStorage.cpp
    :exec: text/string --demo ConstructionAndStorage
    :source-sha256: 915fe8b4889e128bf0588309a976e52260b515fd99b1f6893a31d2a52b5cc59b

.. code-block:: cpp

    /// Get the first word in the given text with letters.
    auto getFirstWordRange(const el::String &text) -> el::ByteRange {
        static const auto separator = el::CharSet::from(el::UnicodeCategoryGroup::Letter);
        const auto begin = text.findFirstOf(separator);
        if (begin.isNoIndex()) {
            return {};
        }
        auto end = text.findFirstNotOf(separator, begin);
        if (end.isNoIndex()) {
            end = text.indexAt(el::StringSide::Back);
        }
        return el::ByteRange{begin, end};
    }

    /// This demo demonstrates how a `String` can be constructed from various sources and how it behaves.
    void constructionAndStorage() {
        // Constructing a `String` will just store a safe reference to the string literal.
        const auto textForRead = el::String{"🌲 Schwarzwald: kühle Morgenluft"_el};
        el::io::printLine("A string from a string literal: "_el, textForRead);
        printMemoryAndRangeInfo(textForRead);
        // A local editor is appropriate when construction and in-place editing are explicit parts of the workflow.
        auto constructedText = el::StringEditor{"🌴 Bali: "_el};
        constructedText.append("hangatnya angin fajar"_el);
        el::io::printLine("A simply constructed string: "_el, constructedText);
        printMemoryAndRangeInfo(constructedText);

        // `String`s are created implicitly from `StringEditor` objects.
        // The read-only string stores an owning reference to the editor's storage.
        // Even when the editor is destroyed, the string and the storage remain valid.
        const el::String stringFromEditor = constructedText;
        // `String`s are also created implicitly from `StringLiteral` objects.
        // That makes them the primary choice for function parameters and to store strings.
        const el::String stringFromLiteral = "🌳 Forêt humide après la pluie"_el;

        // The strings look identical but carry different storage references.
        // From a user perspective, there is no difference in behavior.
        el::io::printLine("String from editor  : "_el, stringFromEditor);
        printMemoryAndRangeInfo(stringFromEditor);
        el::io::printLine("String from literal : "_el, stringFromLiteral);
        printMemoryAndRangeInfo(stringFromLiteral);

        // When sliced, strings keep the same storage reference but change their visible range.
        auto word = stringFromEditor.slice(getFirstWordRange(stringFromEditor));
        el::io::printLine("First word #1 : "_el, word);
        printMemoryAndRangeInfo(word);
        word = stringFromLiteral.slice(getFirstWordRange(stringFromLiteral));
        el::io::printLine("First word #2 : "_el, word);
        printMemoryAndRangeInfo(word);
    }

.. erbsland-ansi::
    :escape-char: ␛

    A string from a string literal: 🌲 Schwarzwald: kühle Morgenluft
            U8String:
                storageKind: literal
                backingStorageId: 0x26235705d65e892d:0x1a0ad4772ab0e6a9
                selectedRange: index: 0 - 35 (length: 35)
    A simply constructed string: 🌴 Bali: hangatnya angin fajar
            U8String:
                storageKind: shared
                backingStorageId: 0x26235705d43aa4ce:0x1a0ad47728d4cb8e
                selectedRange: index: 0 - 32 (length: 32)
    String from editor  : 🌴 Bali: hangatnya angin fajar
            U8String:
                storageKind: shared
                backingStorageId: 0x26235705d43aa4ce:0x1a0ad47728d4cb8e
                selectedRange: index: 0 - 32 (length: 32)
    String from literal : 🌳 Forêt humide après la pluie
            U8String:
                storageKind: literal
                backingStorageId: 0x26235705d65e89a6:0x1a0ad4772ab0e725
                selectedRange: index: 0 - 34 (length: 34)
    First word #1 : Bali
            U8String:
                storageKind: shared
                backingStorageId: 0x26235705d43aa4ce:0x1a0ad47728d4cb8e
                selectedRange: index: 5 - 9 (length: 4)
    First word #2 : Forêt
            U8String:
                storageKind: literal
                backingStorageId: 0x26235705d65e89a6:0x1a0ad4772ab0e725
                selectedRange: index: 5 - 11 (length: 6)

.. erbsland-demo-end::

Let Literals Stay Literal
=========================

The ``_el`` suffix creates an Erbsland string literal with static lifetime.
Passing it directly avoids allocating and copying character data.
Construct a ``String`` from the literal when the value must be stored in a uniform container or returned through a
``String`` interface.

.. erbsland-demo::
    :source: text/String/EfficientLiteralUsage.cpp
    :exec: text/string --demo EfficientLiteralUsage
    :source-sha256: d859bd83d940f2312be3dcf2dc128efc1ae99f6015d86620c48d491e4a00e69f

.. code-block:: cpp

    /// `String` only copies a string when there is no alternative.
    ///
    /// - By default, it keeps an owning reference to the original string and stores a range into that string.
    /// - For string literals in read-only memory, where this is safe, it simply references the literal data.
    ///
    /// This demo splices individual words from a long string literal and sorts them by character length.
    /// The debug output shows that the word views select different ranges in the same backing literal storage.
    ///
    void efficientLiteralUsage() {
        // Prepare a set containing all separators used in the text.
        static auto separatorSet = el::CharSet::fromPattern(" ,.!?\n«»“”"_el);

        // Create a view into the story literal.
        const auto text = el::String{cStory};

        // Prepare a list of words and their character/code point lengths.
        using WordAndLength = std::pair<el::String, el::CpLength>;
        auto wordList = std::vector<WordAndLength>{};

        // Scan through the text, extracting words and skipping separators.
        // Note: there is a `String::split()` method that splits text with one call.
        el::ByteIndex pos = text.indexAt(el::StringSide::Front);
        while (pos < text.indexAt(el::StringSide::Back)) {
            auto wordStart = text.findFirstNotOf(separatorSet, pos);
            if (wordStart.isNoIndex()) {
                break;
            }
            pos = text.findFirstOf(separatorSet, wordStart);
            const auto word = text.slice(el::ByteRange{wordStart, pos});
            wordList.emplace_back(word, word.characterLength());
        }

        // Sort the words by length, then lexicographically.
        std::ranges::stable_sort(wordList, [](const WordAndLength &a, const WordAndLength &b) -> bool {
            if (a.second == b.second) {
                return a.first < b.first;
            }
            return a.second < b.second;
        });

        // Print the sorted list of words.
        el::io::printLine("Sorted word list:"_el);
        const auto integerFormat = el::IntegerFormat::decimal().setFieldWidth(el::CpLength{4});
        auto lastLength = el::CpLength::infinite();
        for (const auto &[word, length] : wordList) {
            if (length != lastLength) {
                lastLength = length;
                el::io::print("\n"_el, integerFormat, length.toRawValue(), ": "_el);
            } else {
                el::io::print(", "_el);
            }
            el::io::print(word);
        }
        el::io::printLine();

        // The memory debug view exposes both the visible range identity and the backing storage identity.
        // The original text and the selected word have different ranges but the same backing storage.
        constexpr auto debugDetails = el::DebugViewDetail::BackingStore;
        el::io::printLine(
            "The output below shows the memory view of the original text and three selected words.\n"_el,
            "Compare the field \"backingStorageId\" - all strings share the original literal data.\n\n"_el,
            "Original text:\n"_el,
            el::toDebugString(text, debugDetails),
            "\nView of three random words:\n"_el);
        for (auto i = 0; i < 3; ++i) {
            const auto index = el::application().random().selectInteger<std::size_t>(std::size_t{0}, wordList.size());
            const auto &entry = wordList.at(index);
            el::io::printLine(
                "Word "_el, index, ": \""_el, entry.first, "\", (length: "_el, entry.second.toRawValue(), ")"_el);
            el::io::printLine(el::toDebugString(wordList.at(index).first, debugDetails));
        }
    }

.. erbsland-ansi::
    :escape-char: ␛

    Sorted word list:

       1: 🌲, 🍄
       2: Im, La, an, in
       3: Die, Léa, Sie, Und, Wei, auf, auf, auf, das, dem, den, den, den, den, den, des, die,
     die, die, ein, und, und, und, von, zum, 🌧️🌲
       4: Dann, Doch, Wald, Wald, doch, fiel, noch, noch, rief, Äste, Über
       5: Ayumi, Neben, Pilze, Sofia, Stein, Vögel, alten, alten, blieb, durch, einem, einer,
    einer, hohen, legte, leise, sagte, zogen, Ästen
       6: Binnen, Bodens, Bächen, Farnen, Gläser, Gruppe, Himmel, Javier, Mehmet, Steine, arri
    ve, hinter, kleine, liefen, offene, rannte, sangen, stehen, voller, vorbei, warmer, weiter
    , weiter, zeigte
       7: Blätter, Camille, Fichten, Tropfen, Wurzeln, bientôt, deutete, dunklen, einfach, kle
    inen, lachend, niemand, schwere, stillen, tempête, weniger, während, während, 观察风向变化
    ！
       8: Sekunden, hindurch, notierte, rauschte, sammelte, zwischen, zwischen
       9: Notizbuch, Plötzlich, einzelner, tanzenden
      10: Temperatur, leuchtende, reparierte
      11: Baumwipfeln, Regenwolken, Sommerregen, glitzernder, nummerierte
      13: Observatorium, moosbedeckten
      26: Windgeschwindigkeitsmesser
    The output below shows the memory view of the original text and three selected words.
    Compare the field "backingStorageId" - all strings share the original literal data.

    Original text:
    U8String:
        backingStorageId: 0x26235705d5255f35:0x1a0ad47729cb3c34
    View of three random words:

    Word 123: "nummerierte", (length: 11)
    U8String:
        backingStorageId: 0x26235705d5255f35:0x1a0ad47729cb3c34
    Word 1: "🍄", (length: 1)
    U8String:
        backingStorageId: 0x26235705d5255f35:0x1a0ad47729cb3c34
    Word 0: "🌲", (length: 1)
    U8String:
        backingStorageId: 0x26235705d5255f35:0x1a0ad47729cb3c34

.. erbsland-demo-end::

Cheap Copies and Long-lived Slices
==================================

Copies and slices retain shared ownership of their backing storage.
A slice therefore remains valid after the source object goes out of scope, without copying the selected character data.
This is especially useful for tokens, fields, and lines extracted from larger documents.

A tiny slice can intentionally keep a large backing allocation alive.
If the slice will outlive the source context and retained memory matters, create an independent compact value at that
boundary.
See :doc:`slicing_and_splitting_strings` for coordinate choices and
:doc:`managing_string_editor_storage` for storage diagnostics.

.. erbsland-demo::
    :source: text/String/StorageIdentifier.cpp
    :exec: text/string --demo StorageIdentifier
    :source-sha256: 695337134bbed5a2b4d9d9b178282dd1db2a876ac7e100e1146cb65f1af683ca

.. code-block:: cpp

    /// `storageId()` lets low-level code verify that a cached native index still
    /// belongs to the same visible storage range.
    ///
    /// This is useful when a byte index outlives the immediate operation that
    /// produced it. Even when two strings contain the same decoded text, a native
    /// index from one storage range must not be applied to another one.
    void storageIdentifier() {
        struct CachedRange final {
            el::StorageIdentifier storageId;
            el::ByteIndex index;
            el::ByteLength length;
        };

        const auto report = el::String{"温度計A: 21℃; 気圧計B: 1012hPa; 湿度計C: 45%"_el};
        const auto token = el::String{"気圧計"_el};
        const auto cachedToken = CachedRange{report.storageId(), report.find(token), token.length()};
        const auto booleanFormat = el::BooleanFormat::yesNo();

        const auto tryUseCachedRange = [&](const el::String &label, const el::String &candidate) -> void {
            const auto sameStorage = candidate.storageId() == cachedToken.storageId;
            el::io::printLine(label, ":"_el);
            el::io::printLine("  same visible storage range: "_el, booleanFormat, sameStorage);
            if (sameStorage && !cachedToken.index.isNoIndex()) {
                el::io::printLine(
                    "  cached range reads ........: "_el,
                    candidate.slice(el::ByteRange{cachedToken.index, cachedToken.length}));
            } else {
                el::io::printLine("  cached range reads ........: <not used>"_el);
            }
        };

        const auto copiedReport = report.copy();
        const auto tail = report.slice(el::ByteRange{report.find(token), el::ByteLength::infinite()});

        el::io::printLine("Report: "_el, report);
        el::io::printLine("Cached token: "_el, token);
        el::io::printLine("Cached byte index: "_el, cachedToken.index);
        el::io::printLine();

        tryUseCachedRange("Original string"_el, report);
        tryUseCachedRange("Copied text"_el, copiedReport);
        tryUseCachedRange("Tail slice"_el, tail);
    }

.. erbsland-ansi::
    :escape-char: ␛

    Report: 温度計A: 21℃; 気圧計B: 1012hPa; 湿度計C: 45%
    Cached token: 気圧計
    Cached byte index: 19

    Original string:
      same visible storage range: yes
      cached range reads ........: 気圧計
    Copied text:
      same visible storage range: no
      cached range reads ........: <not used>
    Tail slice:
      same visible storage range: no
      cached range reads ........: <not used>

.. erbsland-demo-end::
