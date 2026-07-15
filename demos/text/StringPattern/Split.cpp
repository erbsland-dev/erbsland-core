// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// `split()` returns the matched text and the remaining text at the pattern boundary.
///
/// The method is especially handy when `?` or bracket sets make the matched prefix variable in bytes. For a pattern
/// with both a prefix and suffix, the suffix is validated and then dropped from the second result.
void split() {
    const auto recordPattern = el::StringPattern{"probe-??*;ok"_el};
    const auto record = el::StringView{"probe-A7 temperatur=21.4C;ok"_el};

    const auto [recordId, valueText] = recordPattern.split(record);
    el::io::printLine("record id ...............: "_el, recordId);
    el::io::printLine("value ...................: "_el, valueText);

    const auto suffixPattern = el::StringPattern{"*;fehler"_el};
    const auto [payload, suffix] = suffixPattern.split("probe-B8 leitwert=0.41;fehler"_el);
    el::io::printLine("payload .................: "_el, payload);
    el::io::printLine("suffix ..................: "_el, suffix);
}

}
