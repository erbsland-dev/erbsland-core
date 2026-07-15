// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

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

}
