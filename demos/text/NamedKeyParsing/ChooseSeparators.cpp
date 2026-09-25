// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/text/named_key/Format.hpp>
#include <erbsland/text/named_key/Parser.hpp>
#include <erbsland/text/StringCharReader.hpp>

namespace demo {

/// Read a quiz field embedded in a larger text record.
///
/// The list separator divides entries, the value separator divides a key from its value, and the stop character
/// ends this field while leaving the remaining text available to the caller.
void chooseSeparators() {
    const auto format = el::named_key::Format{}
                            .setKeys({{"topic"_el, 0}, {"level"_el, 1}})
                            .setListSeparator(U';')
                            .setValueSeparator(U':')
                            .setStopCharacter(U']')
                            .setValueListAllowed(false);
    auto reader = el::StringCharReader{el::String{"topic:gezegen;level:2]next field"_el}};
    const auto entries = el::named_key::Parser{reader, format}.readAllEntries();
    for (const auto &entry : entries) {
        el::io::printLine(format.keyName(entry.keyIndex()), ": "_el, entry.value());
    }
    el::io::printLine("Next field remains: "_el, reader.peek() == U'n');
}

}
