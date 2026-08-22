// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "HostEndpoint.hpp"

#include "impl/host/CommonHostTests.hpp"

#include "../err/ParseError.hpp"
#include "../text/CharSet.hpp"
#include "../text/IntegerBase.hpp"
#include "../text/IntegerParseOptions.hpp"
#include "../text/Literals.hpp"
#include "../text/StringEditor.hpp"
#include "../text/StringList.hpp"
#include "../unit/CpLength.hpp"
#include "../unit/ItemCount.hpp"
#include "../unit/ItemIndex.hpp"
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
        return fromStringOrThrow(text);
    } catch (const err::ParseError &) {
        return std::nullopt;
    }
}

auto HostEndpoint::fromStringOrThrow(const String &text) -> HostEndpoint {
    impl::testCommonHostText(text, "host end-point"_el);
    auto hostText = String{};
    auto portText = String{};
    auto scopeId = ScopeId{};
    const auto isBracketed = text.charAt(StringSide::Front) == U'[';
    if (isBracketed) {
        const auto bracketParts = StringList::fromSplit(text, CharSet{U']'}, ItemCount{1U}, true);
        if (bracketParts.count() != ItemCount{2U}) {
            throw err::ParseError{"A bracketed host endpoint requires one closing bracket."_el};
        }
        const auto &bracketedHost = bracketParts.get(ItemIndex::zero());
        const auto &afterBracket = bracketParts.get(ItemIndex{1U});
        if (bracketedHost.charAt(StringSide::Front) != U'[' || afterBracket.charAt(StringSide::Front) != U':' ||
            afterBracket.contains("]"_el)) {
            throw err::ParseError{"A bracketed host endpoint must use [host]:port syntax."_el};
        }
        hostText = bracketedHost.slice(StringSide::Back, CpIndex{1U});
        portText = afterBracket.slice(StringSide::Back, CpIndex{1U});
        const auto scopeParts = StringList::fromSplit(hostText, CharSet{U'%'}, ItemCount::infinite(), true);
        if (scopeParts.count() > ItemCount{2U}) {
            throw err::ParseError{"An IPv6 scope identifier may contain only one percent sign."_el};
        }
        if (scopeParts.count() == ItemCount{2U}) {
            auto options = IntegerParseOptions{};
            options.setFixedBase(IntegerBase::Decimal).setMinimumDigits(CpLength::one());
            const auto scopeValue = scopeParts.get(ItemIndex{1U}).toIntegerOrThrow<uint64_t>(options);
            if (scopeValue == 0U || scopeValue > std::numeric_limits<uint32_t>::max()) {
                throw err::ParseError{"The IPv6 scope identifier must be between 1 and 4,294,967,295."_el};
            }
            scopeId = ScopeId{static_cast<uint32_t>(scopeValue)};
            hostText = scopeParts.get(ItemIndex::zero());
        }
    } else {
        const auto parts = StringList::fromSplit(text, CharSet{U':'}, ItemCount::infinite(), true);
        if (parts.count() != ItemCount{2U}) {
            throw err::ParseError{"An unbracketed host endpoint must use host:port syntax."_el};
        }
        hostText = parts.get(ItemIndex::zero());
        portText = parts.get(ItemIndex{1U});
    }
    const auto host = Host::fromStringOrThrow(hostText);
    const auto address = host.address();
    if ((isBracketed && (!address.has_value() || !address->isV6())) ||
        (!isBracketed && address.has_value() && address->isV6()) ||
        (scopeId.isSpecified() && (!address.has_value() || !address->isV6()))) {
        throw err::ParseError{"Only IPv6 addresses may be bracketed or have a scope identifier."_el};
    }
    return HostEndpoint{host, Port::fromStringOrThrow(portText), scopeId};
}

}
