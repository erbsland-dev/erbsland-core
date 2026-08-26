// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "StringDemos.hpp"

namespace demo {

/// Normalize after joining when a new canonical sequence can form at a fragment boundary.
void normalizeJoinedText() {
    const auto station = el::String{"Meetpunt A"_el}.normalized(el::NormalizationForm::Nfc);
    const auto ring = el::String{"\u030A"_el}.normalized(el::NormalizationForm::Nfc);
    const auto joined = el::String::fromJoined({station, ring});
    const auto normalized = joined.normalized(el::NormalizationForm::Nfc);

    el::io::printLine("Separately normalized : "_el, joined);
    el::io::printLine("After joining .........: "_el, normalized);
    el::io::printLine("Code points ..........: "_el, joined.characterLength(), " -> ", normalized.characterLength());
}

}
