// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TlsConfigurationLabel.hpp"

#include "../../err/ParameterFailure.hpp"
#include "../../text/CharSet.hpp"
#include "../../text/Literals.hpp"
#include "../../text/u8/U8StringConstIterator.hpp"

namespace erbsland::cryptology::impl::tls_configuration_label {

using namespace text;
using namespace text::literals;

auto validationError(const String &label) -> String {
    if (label.length().toSizeT() > cMaximumLength) {
        return "A TLS configuration label must not exceed 255 bytes."_el;
    }
    if (label.isEmpty()) {
        return {};
    }
    static const auto cAllowedCharacters = CharSet::fromPattern("a-z0-9/-"_el);
    if (!label.containsOnly(cAllowedCharacters)) {
        return "A TLS configuration label contains an invalid character."_el;
    }
    auto segmentCount = std::size_t{1U};
    auto segmentStart = true;
    auto previousWasHyphen = false;
    for (const auto character : label) {
        if (character == U'/') {
            if (segmentStart || previousWasHyphen) {
                return "A TLS configuration label contains an invalid segment."_el;
            }
            ++segmentCount;
            if (segmentCount > cMaximumSegments) {
                return "A TLS configuration label must not exceed 16 segments."_el;
            }
            segmentStart = true;
            previousWasHyphen = false;
            continue;
        }
        if (segmentStart && character == U'-') {
            return "A TLS configuration label segment cannot begin with a hyphen."_el;
        }
        segmentStart = false;
        previousWasHyphen = character == U'-';
    }
    if (segmentStart || previousWasHyphen) {
        return "A TLS configuration label contains an invalid segment."_el;
    }
    return {};
}

void validate(const String &label) {
    if (const auto reason = validationError(label); !reason.isEmpty()) {
        throw err::ParameterFailure{reason, "label"_el};
    }
}

}
