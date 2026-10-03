// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/AsciiCategory.hpp>
#include <erbsland/CharSet.hpp>
#include <erbsland/ItemIndex.hpp>
#include <erbsland/named_key/Format.hpp>
#include <erbsland/named_key/Parser.hpp>
#include <erbsland/StandardStreams.hpp>
#include <erbsland/String.hpp>
#include <erbsland/StringCharReader.hpp>

namespace demo {

/// Add optional signs and compact numeric values to a quiz field.
///
/// Prefixes are reported separately from the semantic key. A compact-value character marks the start of a value
/// without requiring the normal key/value separator.
void prefixAndCompactValues() {
    const auto format = el::named_key::Format{}
                            .setKeys({{"hint"_el, 0}, {"level"_el, 1}})
                            .setAllowedKeyPrefixes(el::CharSet{U'+', U'-'})
                            .setValueWithoutKeySeparatorChars(el::CharSet::from(el::AsciiCategory::Digit))
                            .setValueListAllowed(false);
    auto reader = el::StringCharReader{"+hint,level3"_el};
    const auto entries = el::named_key::Parser{reader, format}.readAllEntries();
    el::io::printLine("Hint enabled: "_el, entries.get(el::unit::ItemIndex{0U}).prefix() == U'+');
    el::io::printLine("Quiz level: "_el, entries.get(el::unit::ItemIndex{1U}).value());
}

}
