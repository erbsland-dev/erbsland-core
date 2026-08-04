// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "IpEndpoint.hpp"

#include "impl/CommonHostTests.hpp"

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
        return fromStringOrThrow(text);
    } catch (const err::ParseError &) {
        return std::nullopt;
    }
}

auto IpEndpoint::fromStringOrThrow(const String &text) -> IpEndpoint {
    impl::testCommonHostText(text, "IP-endpoint"_el);
    auto addressText = String{};
    auto portText = String{};
    auto scopeId = ScopeId{};
    const auto isBracketed = text.charAt(StringSide::Front) == U'[';
    if (isBracketed) {
        const auto bracketParts = StringList::fromSplit(text, CharSet{U']'}, ItemCount{1U}, true);
        if (bracketParts.count() != ItemCount{2U}) {
            throw err::ParseError{"A bracketed IP endpoint requires one closing bracket."_el};
        }
        const auto &bracketedAddress = bracketParts.get(ItemIndex::zero());
        const auto &afterBracket = bracketParts.get(ItemIndex{1U});
        if (bracketedAddress.charAt(StringSide::Front) != U'[' || afterBracket.charAt(StringSide::Front) != U':' ||
            afterBracket.contains("]"_el)) {
            throw err::ParseError{"A bracketed IP endpoint must use [address]:port syntax."_el};
        }
        addressText = bracketedAddress.slice(StringSide::Back, CpIndex{1U});
        portText = afterBracket.slice(StringSide::Back, CpIndex{1U});
        const auto scopeParts = StringList::fromSplit(addressText, CharSet{U'%'}, ItemCount::infinite(), true);
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
            addressText = scopeParts.get(ItemIndex::zero());
        }
    } else {
        const auto parts = StringList::fromSplit(text, CharSet{U':'}, ItemCount::infinite(), true);
        if (parts.count() != ItemCount{2U}) {
            throw err::ParseError{"An unbracketed IP endpoint must use IPv4-address:port syntax."_el};
        }
        addressText = parts.get(ItemIndex::zero());
        portText = parts.get(ItemIndex{1U});
    }
    const auto address = IpAddress::fromStringOrThrow(addressText);
    if (isBracketed != address.isV6()) {
        throw err::ParseError{"IPv6 addresses must be bracketed and IPv4 addresses must not be bracketed."_el};
    }
    return IpEndpoint{address, Port::fromStringOrThrow(portText), scopeId};
}

}
