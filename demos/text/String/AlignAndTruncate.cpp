// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

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

}
