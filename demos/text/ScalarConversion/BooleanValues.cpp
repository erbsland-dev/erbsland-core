// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ScalarConversionDemos.hpp"

#include <erbsland/BooleanFormat.hpp>
#include <erbsland/Capitalization.hpp>
#include <erbsland/ParseError.hpp>
#include <erbsland/StandardStreams.hpp>
#include <erbsland/String.hpp>

namespace demo {

/// Convert complete Boolean literals and choose the words used for Boolean output.
void booleanValues() {
    const auto onlineText = el::String{"YES"_el};
    const auto sleepingText = el::String{"disabled"_el};
    const auto paddedText = el::String{" yes "_el};

    el::io::printLine("YES .............: "_el, onlineText.toBooleanOrThrow());
    el::io::printLine("disabled ........: "_el, sleepingText.toBooleanOrThrow());
    el::io::printLine("padded fallback .: "_el, paddedText.toBoolean(true));

    try {
        const auto unexpectedValue = el::String{"misschien"_el}.toBooleanOrThrow();
        el::io::printLine("misschien .......: "_el, unexpectedValue);
    } catch (const el::ParseError &) {
        el::io::printLine("misschien .......: parse error"_el);
    }

    auto capitalizedYesNo = el::BooleanFormat::yesNo();
    capitalizedYesNo.setCapitalization(el::Capitalization::Capitalized);
    auto upperEnabled = el::BooleanFormat::enabledDisabled();
    upperEnabled.setCapitalization(el::Capitalization::Uppercase);
    const auto onlineOutput = el::String::fromBoolean(true, capitalizedYesNo);
    const auto offlineOutput = el::String::fromBoolean(false, upperEnabled);

    el::io::printLine("Formatted status words:"_el);
    el::io::printLine("  "_el, onlineOutput);
    el::io::printLine("  "_el, offlineOutput);
}

}
