// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "HostName.hpp"

#include "../err/ParseError.hpp"
#include "../text/punycode/PunycodeDecoder.hpp"
#include "../text/punycode/PunycodeEncoder.hpp"

namespace erbsland::network {

using namespace unit;
using namespace text;
using namespace text::literals;

auto HostName::fromString(const String &text) noexcept -> std::optional<HostName> {
    try {
        return fromStringOrThrow(text);
    } catch (const err::ParseError &) {
        return std::nullopt;
    }
}

auto HostName::fromStringOrThrow(const String &text) -> HostName {
    const auto options = punycode::PunycodeOptions::network();
    const auto idnaAscii = punycode::PunycodeEncoder{text, options}.encodeOrThrow();
    const auto unicode = punycode::PunycodeDecoder{idnaAscii, options}.decodeOrThrow();
    return HostName{unicode, idnaAscii};
}

}
