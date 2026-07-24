// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Host.hpp"

#include "../err/ParseError.hpp"
#include "../util/HashHelper.hpp"

namespace erbsland::network {

using namespace unit;
using namespace text;

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
    if (const auto addressValue = IpAddress::fromString(text); addressValue.has_value()) {
        return Host{*addressValue};
    }
    if (const auto nameValue = HostName::fromString(text); nameValue.has_value()) {
        return Host{*nameValue};
    }
    return std::nullopt;
}

auto Host::fromStringOrThrow(const String &text) -> Host {
    if (const auto result = fromString(text); result.has_value()) {
        return *result;
    }
    throw err::ParseError{"The text is not a valid host."};
}

}
