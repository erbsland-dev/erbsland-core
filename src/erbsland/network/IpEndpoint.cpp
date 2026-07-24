// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "IpEndpoint.hpp"

#include "../err/ParseError.hpp"
#include "../text/CharSet.hpp"
#include "../text/IntegerBase.hpp"
#include "../text/IntegerParseOptions.hpp"
#include "../text/Literals.hpp"
#include "../text/StringEditor.hpp"
#include "../text/StringList.hpp"
#include "../unit/CpLength.hpp"
#include "../unit/ElementCount.hpp"
#include "../unit/ElementIndex.hpp"
#include "../util/HashHelper.hpp"

#include <limits>

namespace erbsland::network {

using namespace unit;
using namespace text;
using namespace text::literals;

auto IpEndpoint::toString() const -> String {
    auto result = StringEditor{};
    if (_address.isV4()) {
        result.append(_address.toString());
        result.append(":"_el);
        result.append(_port.toString());
        return result;
    }
    result.append("["_el);
    result.append(_address.toString());
    if (_scopeId.isSpecified()) {
        result.append("%"_el);
        result.append(String::fromInteger(_scopeId.toRawValue()));
    }
    result.append("]:"_el);
    result.append(_port.toString());
    return result;
}

auto IpEndpoint::toHash() const noexcept -> std::size_t {
    return util::createHash(_address, _port.toRawValue(), _scopeId.toRawValue());
}

auto IpEndpoint::fromString(const String &text) noexcept -> std::optional<IpEndpoint> {
    try {
        auto addressText = String{};
        auto portText = String{};
        auto scopeId = ScopeId{};
        const auto isBracketed = text.charAt(StringSide::Front) == U'[';
        if (isBracketed) {
            const auto bracketParts = StringList::fromSplit(text, CharSet{U']'}, ElementCount{1U}, true);
            if (bracketParts.count() != ElementCount{2U}) {
                return std::nullopt;
            }
            const auto &bracketedAddress = bracketParts.get(ElementIndex::zero());
            const auto &afterBracket = bracketParts.get(ElementIndex{1U});
            if (bracketedAddress.charAt(StringSide::Front) != U'[' || afterBracket.charAt(StringSide::Front) != U':' ||
                afterBracket.contains("]"_el)) {
                return std::nullopt;
            }
            addressText = bracketedAddress.slice(StringSide::Back, CpIndex{1U});
            portText = afterBracket.slice(StringSide::Back, CpIndex{1U});
            const auto scopeParts = StringList::fromSplit(addressText, CharSet{U'%'}, ElementCount::infinite(), true);
            if (scopeParts.count() > ElementCount{2U}) {
                return std::nullopt;
            }
            if (scopeParts.count() == ElementCount{2U}) {
                auto options = IntegerParseOptions{};
                options.setFixedBase(IntegerBase::Decimal).setMinimumDigits(CpLength::one());
                const auto scopeValue = scopeParts.get(ElementIndex{1U}).toInteger<uint64_t>(0U, options);
                if (scopeValue == 0U || scopeValue > std::numeric_limits<uint32_t>::max()) {
                    return std::nullopt;
                }
                scopeId = ScopeId{static_cast<uint32_t>(scopeValue)};
                addressText = scopeParts.get(ElementIndex::zero());
            }
        } else {
            const auto parts = StringList::fromSplit(text, CharSet{U':'}, ElementCount::infinite(), true);
            if (parts.count() != ElementCount{2U}) {
                return std::nullopt;
            }
            addressText = parts.get(ElementIndex::zero());
            portText = parts.get(ElementIndex{1U});
        }
        const auto address = IpAddress::fromString(addressText);
        const auto port = Port::fromString(portText);
        if (!address.has_value() || !port.has_value() || (isBracketed != address->isV6()) ||
            (scopeId.isSpecified() && !address->isV6())) {
            return std::nullopt;
        }
        return IpEndpoint{*address, *port, scopeId};
    } catch (...) {
        return std::nullopt;
    }
}

auto IpEndpoint::fromStringOrThrow(const String &text) -> IpEndpoint {
    if (const auto result = fromString(text); result.has_value()) {
        return *result;
    }
    throw err::ParseError{"The text is not a valid resolved IP endpoint."_el};
}

}
