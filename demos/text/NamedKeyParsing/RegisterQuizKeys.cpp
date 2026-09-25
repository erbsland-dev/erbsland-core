// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/text/named_key/Format.hpp>
#include <erbsland/text/named_key/Parser.hpp>
#include <erbsland/text/StringCharReader.hpp>

namespace demo {

/// Register a vocabulary for a compact quiz request.
///
/// `Format` stores normalized key spellings and integer identifiers. Several spellings can share one identifier,
/// while the first registered spelling is the name returned by `keyName()`.
void registerQuizKeys() {
    enum class QuizKey { Topic, Level };
    auto format = el::named_key::Format{}.setKeys({
        {"topic"_el, static_cast<int>(QuizKey::Topic)},
        {"subject"_el, static_cast<int>(QuizKey::Topic)},
    });
    format.addKey("level"_el, static_cast<int>(QuizKey::Level));
    format.setValueListAllowed(false);

    // The spelling in the input is normalized before lookup.
    auto reader = el::StringCharReader{el::String{"SUBJECT=gezegen,level=2"_el}};
    for (const auto &entry : el::named_key::Parser{reader, format}.readAllEntries()) {
        el::io::printLine(format.keyName(entry.keyIndex()), ": "_el, entry.value());
    }
    el::io::printLine("Registered aliases: "_el, format.keys().count());
}

}
