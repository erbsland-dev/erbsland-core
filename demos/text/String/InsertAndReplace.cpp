// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

/// `insert()` adds text at a byte or code-point index. `replace()` changes a
/// range, `replaceFirst()` changes the first matching text occurrence, and
/// `replaceAll()` changes every matching text occurrence or character from a
/// character set.
///
/// Prefer byte indexes when they come from a search operation. Use code-point
/// indexes and ranges when the text is short and the edit position is naturally
/// counted in decoded characters.
void insertAndReplace() {
    auto report = "Plot 07 | sky=grey | sky=grey"_els;

    // A code-point index is readable for inserting at the beginning.
    report.insert(el::CpIndex{0U}, "☀ "_el);

    // Search results are byte indexes and can be reused in a byte range.
    const auto plotNumber = report.find("07"_el);
    report.replace(el::ByteRange{plotNumber, plotNumber + "07"_el.length()}, "08"_el);

    // First and all variants make common text substitutions explicit.
    report.replaceFirst("grey"_el, "clear"_el);
    report.replaceAll("sky="_el, "himmel="_el);

    // A character set replacement handles multiple separators in one pass.
    report.replaceAll(el::CharSet{"|="_el}, U'·');

    auto token = "AβC"_els;
    token.replace(el::CpRange{el::CpIndex{1U}, el::CpLength{1U}}, "beta"_el);

    el::io::printLine("Edited report: "_el, report);
    el::io::printLine("Edited token: "_el, token);
}
