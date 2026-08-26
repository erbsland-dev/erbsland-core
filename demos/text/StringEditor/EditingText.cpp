// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

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

}
