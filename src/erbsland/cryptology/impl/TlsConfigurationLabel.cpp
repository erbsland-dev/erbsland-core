// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TlsConfigurationLabel.hpp"

#include "../../err/ParameterError.hpp"
#include "../../text/CharSet.hpp"
#include "../../text/Literals.hpp"
#include "../../text/u8/U8StringConstIterator.hpp"

namespace erbsland::cryptology::impl::tls_configuration_label {

using namespace text::literals;

void validate(const text::String &label) {
    if (label.length().toSizeT() > cMaximumLength) {
        throw err::ParameterError{"A TLS configuration label must not exceed 255 bytes."_el, "label"_el};
    }
    if (label.isEmpty()) {
        return;
    }
    static const auto cAllowedCharacters = text::CharSet::fromPattern("a-z0-9/-"_el);
    if (!label.containsOnly(cAllowedCharacters)) {
        throw err::ParameterError{"A TLS configuration label contains an invalid character."_el, "label"_el};
    }
    auto segmentCount = std::size_t{1U};
    auto segmentStart = true;
    auto previousWasHyphen = false;
    for (const auto character : label) {
        if (character == U'/') {
            if (segmentStart || previousWasHyphen) {
                throw err::ParameterError{"A TLS configuration label contains an invalid segment."_el, "label"_el};
            }
            ++segmentCount;
            if (segmentCount > cMaximumSegments) {
                throw err::ParameterError{"A TLS configuration label must not exceed 16 segments."_el, "label"_el};
            }
            segmentStart = true;
            previousWasHyphen = false;
            continue;
        }
        if (segmentStart && character == U'-') {
            throw err::ParameterError{"A TLS configuration label segment cannot begin with a hyphen."_el, "label"_el};
        }
        segmentStart = false;
        previousWasHyphen = character == U'-';
    }
    if (segmentStart || previousWasHyphen) {
        throw err::ParameterError{"A TLS configuration label contains an invalid segment."_el, "label"_el};
    }
}

}
