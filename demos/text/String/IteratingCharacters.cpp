// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/Char.hpp>
#include <erbsland/CpCount.hpp>
#include <erbsland/LoopStatus.hpp>
#include <erbsland/StandardStreams.hpp>
#include <erbsland/String.hpp>
#include <erbsland/StringEditor.hpp>
#include <erbsland/U8StringConstIterator.hpp>

namespace demo {

/// `String::forEach()` and range-based `for` loops decode text as Unicode
/// code points without exposing UTF-8 byte boundaries.
///
/// Use `forEach()` when the callback may stop early with `LoopStatus::Stop`.
/// Use a range-based `for` loop when all decoded characters should be visited
/// and the loop body is clearer than a callback.
void iteratingCharacters() {
    const auto plan = el::String{"matematik:45;fen:30;müzik:20"_el};

    auto lettersBeforeBreak = el::CpCount::zero();
    auto digitsBeforeBreak = el::CpCount::zero();

    // Stop as soon as the first lesson entry ends.
    plan.forEach([&](const el::Char character) -> el::LoopStatus {
        if (character == U';') {
            return el::LoopStatus::Stop;
        }
        if (character.isAsciiLetter()) {
            ++lettersBeforeBreak;
        } else if (character.isAsciiDigit()) {
            ++digitsBeforeBreak;
        }
        return el::LoopStatus::Continue;
    });

    el::io::printLine("Before the first separator:"_el);
    el::io::printLine("  ASCII letters: "_el, lettersBeforeBreak);
    el::io::printLine("  ASCII digits: "_el, digitsBeforeBreak);

    auto uppercased = el::StringEditor{};

    // Range-based iteration is compact when every decoded character is needed.
    // For case conversion alone, use `el::toUppercase()` or `el::toLowercase()`.
    for (const auto character : el::String{"ödev: çizim"_el}) {
        uppercased.append(character.toUppercase());
    }

    el::io::printLine("Uppercase walk: "_el, uppercased);
}

}
