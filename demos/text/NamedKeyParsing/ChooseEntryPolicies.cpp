// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/text/named_key/Format.hpp>
#include <erbsland/text/named_key/Parser.hpp>
#include <erbsland/text/StringCharReader.hpp>

namespace demo {

/// Choose which forms of quiz options a field accepts.
///
/// The entry-form switches are independent: repeated keys, bare keys, keyed values, and positional values can
/// each be enabled or disabled according to the field's grammar.
void chooseEntryPolicies() {
    const auto base = el::named_key::Format{}.setKeys({{"topic"_el, 0}, {"hint"_el, 1}}).setValueListAllowed(false);

    // A repeated semantic key can represent several requested topics.
    const auto repeated = el::named_key::Format{base}.setUniqueKeysRequired(false).setValueListAllowed(false);
    auto repeatedReader = el::StringCharReader{el::String{"topic=gezegen,topic=yıldız"_el}};
    el::io::printLine("Topics: "_el, el::named_key::Parser{repeatedReader, repeated}.readAllEntries().count());

    // Require a value after each recognized key.
    const auto valued = el::named_key::Format{base}.setKeysWithoutValuesAllowed(false);
    auto valuedReader = el::StringCharReader{el::String{"topic=gezegen"_el}};
    el::io::printLine("Valued key: "_el, el::named_key::Parser{valuedReader, valued}.readEntry().isKeyWithValue());

    // A flags-only field accepts a bare key but no keyed value.
    const auto flags = el::named_key::Format{base}.setValuesAllowed(false);
    auto flagReader = el::StringCharReader{el::String{"hint"_el}};
    el::io::printLine("Bare flag: "_el, el::named_key::Parser{flagReader, flags}.readEntry().isKey());

    // A positional-answer field accepts a value without a registered key.
    const auto answers = el::named_key::Format{base}.setValueListAllowed(true);
    auto answerReader = el::StringCharReader{el::String{"gezegen"_el}};
    el::io::printLine("Positional answer: "_el, el::named_key::Parser{answerReader, answers}.readEntry().isValue());
}

}
