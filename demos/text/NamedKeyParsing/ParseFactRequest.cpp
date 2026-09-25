// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/text/named_key/Format.hpp>
#include <erbsland/text/named_key/Parser.hpp>
#include <erbsland/text/StringCharReader.hpp>

namespace demo {

/// Parse a compact request for a fact card using semantic key identifiers.
///
/// `Format` maps each accepted spelling to an enum value. `Parser` reads entries from a `StringCharReader`; an
/// alias can share an identifier with its full name. The application interprets the returned entry kinds and values.
void parseFactRequest() {
    enum class FactOption { Subject, Language, Brief };
    const auto format = el::named_key::Format{}
                            .setKeys({
                                {"subject"_el, static_cast<int>(FactOption::Subject)},
                                {"language"_el, static_cast<int>(FactOption::Language)},
                                {"lang"_el, static_cast<int>(FactOption::Language)},
                                {"brief"_el, static_cast<int>(FactOption::Brief)},
                            })
                            .setValueListAllowed(false);
    auto reader = el::StringCharReader{el::String{"subject=revontulet,lang=fi,brief"_el}};
    auto parser = el::named_key::Parser{reader, format};

    // Translate parsed entries into the application's options.
    auto subject = el::String{};
    auto language = el::String{};
    auto brief = false;
    auto valid = true;
    for (const auto &entry : parser.readAllEntries()) {
        switch (static_cast<FactOption>(entry.keyIndex())) {
        case FactOption::Subject:
            valid = valid && entry.isKeyWithValue();
            subject = entry.value();
            break;
        case FactOption::Language:
            valid = valid && entry.isKeyWithValue();
            language = entry.value();
            break;
        case FactOption::Brief:
            valid = valid && entry.isKey();
            brief = entry.isKey();
            break;
        }
    }
    if (!valid || subject.isEmpty() || language.isEmpty()) {
        el::io::printLine("Invalid fact request"_el);
        return;
    }
    el::io::printLine("Subject: "_el, subject);
    el::io::printLine("Language: "_el, language);
    el::io::printLine("Brief: "_el, brief);
}

}
