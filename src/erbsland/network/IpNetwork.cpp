// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "IpNetwork.hpp"

#include "impl/CommonHostTests.hpp"

#include "../err/ParameterError.hpp"
#include "../err/ParseError.hpp"
#include "../text/CharSet.hpp"
#include "../text/IntegerBase.hpp"
#include "../text/IntegerParseOptions.hpp"
#include "../text/Literals.hpp"
#include "../text/StringEditor.hpp"
#include "../text/StringList.hpp"
#include "../unit/ByteIndex.hpp"
#include "../unit/CpLength.hpp"
#include "../unit/ItemCount.hpp"
#include "../unit/ItemIndex.hpp"
#include "../util/HashHelper.hpp"

namespace erbsland::network {

using namespace unit;
using namespace text;
using namespace text::literals;

auto IpNetwork::maximumPrefix(const IpVersion version) noexcept -> uint8_t {
    return version == IpVersion::V4 ? uint8_t{32U} : uint8_t{128U};
}

auto IpNetwork::normalizedAddress(const IpAddress &address, const uint8_t prefixLength) noexcept -> IpAddress {
    auto bytes = address.bytes();
    const auto byteCount = address.isV4() ? 4U : bytes.span().size();
    for (auto index = std::size_t{0}; index < byteCount; ++index) {
        const auto firstBit = index * 8U;
        if (prefixLength >= firstBit + 8U) {
            continue;
        }
        if (prefixLength <= firstBit) {
            bytes.set(ByteIndex::fromSizeT(index), {});
            continue;
        }
        const auto bits = static_cast<uint8_t>(prefixLength - firstBit);
        const auto mask = mem::Byte{static_cast<uint8_t>(0xffU << (8U - bits))};
        bytes.set(ByteIndex::fromSizeT(index), bytes.get(ByteIndex::fromSizeT(index)) & mask);
    }
    return IpAddress::fromBytes(address.version(), bytes);
}

IpNetwork::IpNetwork(IpAddress address, const uint8_t prefixLength) :
    _address{normalizedAddress(address, prefixLength)}, _prefixLength{prefixLength} {
    if (prefixLength > maximumPrefix(address.version())) {
        throw err::ParameterError{"The prefix length is invalid for this IP version."_el, "prefixLength"_el};
    }
}

auto IpNetwork::lastAddress() const noexcept -> IpAddress {
    auto bytes = _address.bytes();
    const auto byteCount = _address.isV4() ? 4U : bytes.span().size();
    for (auto index = std::size_t{0}; index < byteCount; ++index) {
        const auto firstBit = index * 8U;
        if (_prefixLength >= firstBit + 8U) {
            continue;
        }
        if (_prefixLength <= firstBit) {
            bytes.set(ByteIndex::fromSizeT(index), mem::Byte{0xffU});
            continue;
        }
        const auto bits = static_cast<uint8_t>(_prefixLength - firstBit);
        const auto mask = mem::Byte{static_cast<uint8_t>(0xffU >> bits)};
        bytes.set(ByteIndex::fromSizeT(index), bytes.get(ByteIndex::fromSizeT(index)) | mask);
    }
    return IpAddress::fromBytes(_address.version(), bytes);
}

auto IpNetwork::contains(const IpAddress &address) const noexcept -> bool {
    return address.version() == _address.version() && normalizedAddress(address, _prefixLength) == _address;
}

auto IpNetwork::contains(const IpNetwork &network) const noexcept -> bool {
    return network.address().version() == _address.version() && network.prefixLength() >= _prefixLength &&
        contains(network.address());
}

auto IpNetwork::toString() const -> String {
    auto result = StringEditor{};
    result.append(_address.toString());
    result.append("/"_el);
    result.append(String::fromInteger(_prefixLength));
    return result;
}

auto IpNetwork::toHash() const noexcept -> std::size_t {
    return util::createHash(_address, _prefixLength);
}

auto IpNetwork::fromString(const String &text) noexcept -> std::optional<IpNetwork> {
    try {
        return fromStringOrThrow(text);
    } catch (const err::ParseError &) {
        return std::nullopt;
    }
}

auto IpNetwork::fromStringOrThrow(const String &text) -> IpNetwork {
    impl::testCommonHostText(text, "IP-network"_el);
    const auto parts = StringList::fromSplit(text, CharSet{U'/'}, ItemCount::infinite(), true);
    if (parts.count() != ItemCount{2U}) {
        throw err::ParseError{"An IP network must use address/prefix-length syntax."_el};
    }
    const auto address = IpAddress::fromStringOrThrow(parts.get(ItemIndex::zero()));
    auto options = IntegerParseOptions{};
    options.setFixedBase(IntegerBase::Decimal).setMinimumDigits(CpLength::one());
    const auto prefix = parts.get(ItemIndex{1U}).toIntegerOrThrow<unsigned>(options);
    if (prefix > maximumPrefix(address.version())) {
        throw err::ParseError{"The CIDR prefix length exceeds the address-family limit."_el};
    }
    return IpNetwork{address, static_cast<uint8_t>(prefix)};
}

}
