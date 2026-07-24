// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "HostEndpoint.hpp"

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

auto HostEndpoint::toString() const -> String {
    const auto address = _host.address();
    auto result = StringEditor{};
    if (!address.has_value() || address->isV4()) {
        result.append(_host.toString());
        result.append(":"_el);
        result.append(_port.toString());
        return result;
    }
    result.append("["_el);
    result.append(_host.toString());
    if (_scopeId.isSpecified()) {
        result.append("%"_el);
        result.append(String::fromInteger(_scopeId.toRawValue()));
    }
    result.append("]:"_el);
    result.append(_port.toString());
    return result;
}

auto HostEndpoint::toHash() const noexcept -> std::size_t {
    return util::createHash(_host, _port.toRawValue(), _scopeId.toRawValue());
}

auto HostEndpoint::fromString(const String &text) noexcept -> std::optional<HostEndpoint> {
    try {
        auto hostText = String{};
        auto portText = String{};
        auto scopeId = ScopeId{};
        const auto isBracketed = text.charAt(StringSide::Front) == U'[';
        if (isBracketed) {
            const auto bracketParts = StringList::fromSplit(text, CharSet{U']'}, ElementCount{1U}, true);
            if (bracketParts.count() != ElementCount{2U}) {
                return std::nullopt;
            }
            const auto &bracketedHost = bracketParts.get(ElementIndex::zero());
            const auto &afterBracket = bracketParts.get(ElementIndex{1U});
            if (bracketedHost.charAt(StringSide::Front) != U'[' || afterBracket.charAt(StringSide::Front) != U':' ||
                afterBracket.contains("]"_el)) {
                return std::nullopt;
            }
            hostText = bracketedHost.slice(StringSide::Back, CpIndex{1U});
            portText = afterBracket.slice(StringSide::Back, CpIndex{1U});
            const auto scopeParts = StringList::fromSplit(hostText, CharSet{U'%'}, ElementCount::infinite(), true);
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
                hostText = scopeParts.get(ElementIndex::zero());
            }
        } else {
            const auto parts = StringList::fromSplit(text, CharSet{U':'}, ElementCount::infinite(), true);
            if (parts.count() != ElementCount{2U}) {
                return std::nullopt;
            }
            hostText = parts.get(ElementIndex::zero());
            portText = parts.get(ElementIndex{1U});
        }
        const auto host = Host::fromString(hostText);
        const auto port = Port::fromString(portText);
        if (!host.has_value() || !port.has_value()) {
            return std::nullopt;
        }
        const auto address = host->address();
        if ((isBracketed && (!address.has_value() || !address->isV6())) ||
            (!isBracketed && address.has_value() && address->isV6()) ||
            (scopeId.isSpecified() && (!address.has_value() || !address->isV6()))) {
            return std::nullopt;
        }
        return HostEndpoint{*host, *port, scopeId};
    } catch (const err::Exception &) {
        return std::nullopt;
    }
}

auto HostEndpoint::fromStringOrThrow(const String &text) -> HostEndpoint {
    if (const auto result = fromString(text); result.has_value()) {
        return *result;
    }
    throw err::ParseError{"The text is not a valid host endpoint."_el};
}

}
