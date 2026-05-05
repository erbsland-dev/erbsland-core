// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

/// `CharSet` can be created from text or from a list of decoded characters.
///
/// Duplicate characters are ignored, and the resulting set is normalized for efficient membership tests.
void characterSetText() {
    auto punctuation = el::CharSet{"!?.,;"_el};
    auto separators = el::CharSet{U',', U';', U':', U'/'};

    const auto message = el::StringView{"statut: prêt; orbite stable."_el};
    const auto yesNo = el::BooleanFormat::yesNo();
    el::io::printLine("Message contains punctuation : "_el, yesNo, message.containsOneOf(punctuation));
    el::io::printLine("Message contains separators .: "_el, yesNo, message.containsOneOf(separators));
    el::io::printLine("Separator accepts '/' .......: "_el, yesNo, separators.contains(U'/'));
}
