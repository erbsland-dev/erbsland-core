// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

/// `length()` returns the native storage length, while `characterLength()`
/// returns the decoded Unicode code-point length.
///
/// UTF-8 counts bytes, UTF-16 counts 16-bit data units, and UTF-32 counts code
/// points directly. For non-ASCII text these values often differ. Use the native
/// length for storage ranges and indexes, and use the character length only when
/// a user-visible code-point count is the actual question.
void lengthAttributes() {
    const auto u8Reading = el::StringView{"温度計📡: 21℃"_el};
    const auto u16Reading = el::U16StringView{u"温度計📡: 21℃"_el};
    const auto u32Reading = el::U32StringView{U"温度計📡: 21℃"_el};
    const auto asciiLabel = el::StringView{"sensor-21"_el};

    el::io::printLine("Measurement label: "_el, u8Reading);
    el::io::printLine("UTF-8 native length ....: "_el, u8Reading.length());
    el::io::printLine("UTF-8 code points ......: "_el, u8Reading.characterLength());
    el::io::printLine("UTF-16 native length ...: "_el, u16Reading.length());
    el::io::printLine("UTF-16 code points .....: "_el, u16Reading.characterLength());
    el::io::printLine("UTF-32 code points .....: "_el, u32Reading.length());

    el::io::printLine();
    el::io::printLine("ASCII label: "_el, asciiLabel);
    el::io::printLine("ASCII bytes ............: "_el, asciiLabel.length());
    el::io::printLine("ASCII code points ......: "_el, asciiLabel.characterLength());
}
