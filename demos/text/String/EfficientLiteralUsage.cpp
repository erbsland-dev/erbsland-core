// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/debug/StringDebug.hpp>

#include <algorithm>
#include <ranges>

namespace demo {

constexpr static auto cStory = "🌲 Im stillen Wald hinter dem alten Observatorium sammelte Léa leuchtende Blätter, "
                               "während Mehmet den Windgeschwindigkeitsmesser reparierte. Über den Baumwipfeln "
                               "zogen schwere Regenwolken auf, doch zwischen den Ästen sangen die Vögel weiter. "
                               "«La tempête arrive bientôt», sagte Camille leise und zeigte zum dunklen Himmel. "
                               "Neben einem moosbedeckten Stein notierte Javier die Temperatur des Bodens, "
                               "während Ayumi kleine Pilze in nummerierte Gläser legte 🍄.\n"
                               "Plötzlich fiel ein einzelner Tropfen auf das offene Notizbuch von Sofia.\n"
                               "Dann noch einer. Und noch einer.\n"
                               "Binnen weniger Sekunden rauschte warmer Sommerregen durch den Wald. "
                               "Die Gruppe rannte lachend zwischen den hohen Fichten hindurch, "
                               "vorbei an Farnen, alten Wurzeln und kleinen Bächen voller glitzernder Steine.\n"
                               "“观察风向变化！” rief Wei und deutete auf die tanzenden Äste.\n"
                               "Doch niemand blieb stehen.\n"
                               "Sie liefen einfach weiter 🌧️🌲."_el;

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

}
