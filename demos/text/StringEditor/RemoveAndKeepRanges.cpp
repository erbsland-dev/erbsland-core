// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// Range-based in-place edits work best with byte indexes returned by search
/// functions. Those indexes already point into the native UTF-8 storage and can
/// be passed directly to `ByteRange`.
///
/// Code-point ranges are useful for short, fixed-shape labels where positions
/// are naturally counted as decoded characters. For long UTF-8 text, byte
/// ranges avoid repeated scans from the beginning of the string.
void removeAndKeepRanges() {
    const auto record = el::String{"site=Åsen|weather=klart|note=sol"_el};

    // Search returns byte indexes, so the field can be removed without conversion.
    const auto weatherStart = record.find("weather="_el);
    const auto noteStart = record.find("note="_el);
    auto compactRecord = el::StringEditor{record};
    compactRecord.remove(el::ByteRange{weatherStart, noteStart});

    // Keep only the field value by reusing byte indexes from the same search path.
    const auto siteValueStart = record.find("="_el) + "="_el.length();
    const auto siteValueEnd = record.find("|"_el, siteValueStart);
    auto siteName = el::StringEditor{record};
    siteName.keep(el::ByteRange{siteValueStart, siteValueEnd});

    // Code-point ranges are readable for small labels with fixed structure.
    auto label = el::StringEditor{"🌙Luna-04"_el};
    label.keep(el::CpRange{el::CpIndex{1U}, el::CpLength{4U}});

    el::io::printLine("Original: "_el, record);
    el::io::printLine("After remove: "_el, compactRecord);
    el::io::printLine("Kept site: "_el, siteName);
    el::io::printLine("Kept label text: "_el, label);
}

}
