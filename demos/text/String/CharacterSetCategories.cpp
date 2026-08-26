// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// Use `AsciiCategory` directly when an API accepts it, and construct a `CharSet` only when the policy must be retained
/// or combined with other characters.
void characterSetCategories() {
    const auto packetId = el::String{"SENSOR-07"_el};
    const auto packetIdIsValid = packetId.containsOnly(el::AsciiCategory::WordWithHyphen);
    const auto firstIsUppercase =
        packetId.charAt(el::StringSide::Front).isAsciiCategory(el::AsciiCategory::UppercaseLetter);

    static const auto unicodeDigits = el::CharSet::from(el::UnicodeCategory::DecimalNumber);
    static const auto unicodeLetters = el::CharSet::from(el::UnicodeCategoryGroup::Letter);

    const auto yesNo = el::BooleanFormat::yesNo();
    el::io::printLine("Packet-id is ASCII word ..: "_el, yesNo, packetIdIsValid);
    el::io::printLine("First letter is uppercase : "_el, yesNo, firstIsUppercase);
    el::io::printLine("Unicode digit accepts '7' : "_el, yesNo, unicodeDigits.contains(U'7'));
    el::io::printLine("Unicode letter accepts 'é': "_el, yesNo, unicodeLetters.contains(U'é'));
}

}
