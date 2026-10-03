// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/CpCount.hpp>
#include <erbsland/ItemCount.hpp>
#include <erbsland/named_key/Format.hpp>
#include <erbsland/named_key/Parser.hpp>
#include <erbsland/ParseError.hpp>
#include <erbsland/StandardStreams.hpp>
#include <erbsland/String.hpp>
#include <erbsland/StringCharReader.hpp>

namespace demo {

/// Bound the number and length of positional quiz answers.
///
/// `setMaximumValues()` counts positional entries, while `setMaximumValueLength()` measures the decoded value of
/// every positional or keyed entry in Unicode code points.
void limitAnswerValues() {
    const auto format =
        el::named_key::Format{}.setMaximumValues(el::unit::ItemCount{2U}).setMaximumValueLength(el::unit::CpCount{5U});
    auto acceptedReader = el::StringCharReader{"Mars,Venüs"_el};
    el::io::printLine("Accepted answers: "_el, el::named_key::Parser{acceptedReader, format}.readAllEntries().count());

    // A third positional entry exceeds the count limit.
    auto extraReader = el::StringCharReader{"Mars,Venüs,Ay"_el};
    try {
        const auto entries = el::named_key::Parser{extraReader, format}.readAllEntries();
        el::io::printLine("Unexpected accepted answers: "_el, entries.count());
    } catch (const el::err::ParseError &) {
        el::io::printLine("Third answer rejected"_el);
    }

    // The length limit also applies to a value attached to a key.
    auto keyedFormat = format;
    keyedFormat.addKey("topic"_el, 0);
    auto longReader = el::StringCharReader{"topic=gezegen"_el};
    try {
        const auto entry = el::named_key::Parser{longReader, keyedFormat}.readEntry();
        el::io::printLine("Unexpected accepted topic: "_el, entry.value());
    } catch (const el::err::ParseError &) {
        el::io::printLine("Long topic rejected"_el);
    }
}

}
