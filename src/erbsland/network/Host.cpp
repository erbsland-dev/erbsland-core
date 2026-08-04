// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Host.hpp"

#include "impl/CommonHostTests.hpp"

#include "../err/ParseError.hpp"
#include "../util/HashHelper.hpp"

namespace erbsland::network {

using namespace unit;
using namespace text;
using namespace text::literals;

auto Host::address() const noexcept -> std::optional<IpAddress> {
    if (const auto value = std::get_if<IpAddress>(&_value)) {
        return *value;
    }
    return std::nullopt;
}

auto Host::name() const noexcept -> std::optional<HostName> {
    if (const auto value = std::get_if<HostName>(&_value)) {
        return *value;
    }
    return std::nullopt;
}

auto Host::toString() const -> String {
    if (const auto addressValue = address(); addressValue.has_value()) {
        return addressValue->toString();
    }
    if (const auto nameValue = name(); nameValue.has_value()) {
        return nameValue->toString();
    }
    return String{};
}

auto Host::toHash() const noexcept -> std::size_t {
    if (const auto addressValue = address(); addressValue.has_value()) {
        return util::createHash(0U, *addressValue);
    }
    if (const auto nameValue = name(); nameValue.has_value()) {
        return util::createHash(1U, *nameValue);
    }
    return util::createHash(2U);
}

auto Host::fromString(const String &text) noexcept -> std::optional<Host> {
    try {
        return fromStringOrThrow(text);
    } catch (const err::ParseError &) {
        return std::nullopt;
    }
}

auto Host::fromStringOrThrow(const String &text) -> Host {
    impl::testCommonHostText(text, "host name or IP-address"_el);
    if (text.slice(StringSide::Front, CpLength{5U}).contains(":"_el)) {
        return Host{IpAddress::fromStringOrThrow(text)};
    }
    if (const auto address = IpAddress::fromString(text); address.has_value()) {
        return Host{*address};
    }
    return Host{HostName::fromStringOrThrow(text)};
}

}
