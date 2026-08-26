// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// `StringFormat` stores a reusable formatting pattern.
/// Named specifications keep the expected value type and each formatting choice visible.
///
/// Use `build()` to create a new string from formatted values.
/// Use `appendTo()` to add formatted text to an existing `AnyStringBuilder` without
/// creating temporary strings.
void formattingPatterns() {
    // Create a reusable pattern for ISO 8601 date-time values.
    const auto isoDateTime =
        el::StringFormat{"{:number:width=4,zero-fill}-{:number:width=2,zero-fill}-{:number:width=2,zero-fill}T"
                         "{:number:width=2,zero-fill}:{:number:width=2,zero-fill}:{:number:width=2,zero-fill}"_el};

    auto timestamp = isoDateTime.build(2026, 5, 30, 21, 41, 56);
    el::io::printLine("ISO date-time: "_el, timestamp);

    // Create a pattern for simple HTML tags.
    const auto htmlTag = el::StringFormat{"<{0}>{1:text:escape=html}</{0}>\n"_el};

    el::AnyStringBuilder htmlOutput;
    htmlTag.appendTo(htmlOutput, "h1"_el, "Hello World"_el);
    htmlTag.appendTo(htmlOutput, "p"_el, "This paragraph was appended to a string builder."_el);
    htmlTag.appendTo(htmlOutput, "p"_el, "We add another <p> tag with \"useful\" text."_el);

    el::io::printLine("HTML output:"_el);
    el::io::print(htmlOutput);
}

}
