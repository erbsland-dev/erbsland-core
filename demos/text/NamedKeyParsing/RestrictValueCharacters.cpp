// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/err/ParseError.hpp>
#include <erbsland/text/CharSet.hpp>
#include <erbsland/text/named_key/Format.hpp>
#include <erbsland/text/named_key/Parser.hpp>
#include <erbsland/text/StringCharReader.hpp>

namespace demo {

/// Limit quiz answer codes to a defined character set.
///
/// `setAllowedValueChars()` checks every character in a value, after the parser has selected a keyed or positional
/// entry. An empty set leaves values unrestricted by this particular rule.
void restrictValueCharacters() {
    const auto format = el::named_key::Format{}
                            .setKeys({{"answer"_el, 0}})
                            .setAllowedValueChars(el::CharSet::fromPattern("a-z0-9"_el))
                            .setValueListAllowed(false);
    auto acceptedReader = el::StringCharReader{el::String{"answer=gezegen2"_el}};
    el::io::printLine("Answer: "_el, el::named_key::Parser{acceptedReader, format}.readEntry().value());

    // A localized answer with a non-ASCII letter needs a broader allowed set.
    auto rejectedReader = el::StringCharReader{el::String{"answer=güneş"_el}};
    try {
        const auto entry = el::named_key::Parser{rejectedReader, format}.readEntry();
        el::io::printLine("Unexpected accepted answer: "_el, entry.value());
    } catch (const el::err::ParseError &) {
        el::io::printLine("Localized answer rejected"_el);
    }
}

}
