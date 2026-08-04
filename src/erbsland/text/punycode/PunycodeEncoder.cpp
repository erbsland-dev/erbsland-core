// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PunycodeEncoder.hpp"

#include "impl/IdnaProcessor.hpp"
#include "impl/PunycodeCodec.hpp"

#include "../../err/ParseError.hpp"

#include <utility>

namespace erbsland::text::punycode {

PunycodeEncoder::PunycodeEncoder(String text, PunycodeOptions options) :
    _text{std::move(text)}, _options{std::move(options)} {
}

auto PunycodeEncoder::encode() const -> std::optional<String> {
    try {
        return encodeOrThrow();
    } catch (const err::ParseError &) {
        return {};
    }
}

auto PunycodeEncoder::encodeOrThrow() const -> String {
    if (_options.mode() == PunycodeMode::Pure) {
        return impl::encodePunycodePayload(_text);
    }
    return impl::IdnaProcessor{_options}.toAscii(_text);
}

}
