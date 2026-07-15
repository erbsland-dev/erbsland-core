// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// `CharSet::from()` creates reusable sets from ASCII and Unicode character categories.
///
/// ASCII categories are compact and do not require Unicode metadata.
/// Unicode categories are useful when a validation rule should follow standard Unicode character classes.
void characterSetCategories() {
    static const auto asciiHexDigits = el::CharSet::from(el::AsciiCategory::HexDigit);
    static const auto unicodeDigits = el::CharSet::from(el::UnicodeCategory::DecimalNumber);
    static const auto unicodeLetters = el::CharSet::from(el::UnicodeCategoryGroup::Letter);

    const auto yesNo = el::BooleanFormat::yesNo();
    el::io::printLine("ASCII hex accepts 'F' ....: "_el, yesNo, asciiHexDigits.contains(U'F'));
    el::io::printLine("ASCII hex accepts 'G' ....: "_el, yesNo, asciiHexDigits.contains(U'G'));
    el::io::printLine("Unicode digit accepts '7' : "_el, yesNo, unicodeDigits.contains(U'7'));
    el::io::printLine("Unicode letter accepts 'é': "_el, yesNo, unicodeLetters.contains(U'é'));
}

}
