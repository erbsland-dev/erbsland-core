// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// `aligned()` returns a padded read-only value, while `truncate()` shortens an
/// editor in place. Both use decoded code-point lengths rather than UTF-8 bytes.
void alignAndTruncate() {
    const auto label = el::String{"Alpenrose"_el};
    el::io::printLine("Aligned: |"_el, label.aligned(el::CpLength{14U}, el::Alignment::HCenter, U'.'), "|"_el);

    auto observation = el::StringEditor{"Observation: Alpenrose beside pale limestone under morning light"_el};
    observation.truncate(el::CpLength{28U}, el::TruncateMode::Middle, "..."_el);
    el::io::printLine("Truncated: "_el, observation);
}

}
