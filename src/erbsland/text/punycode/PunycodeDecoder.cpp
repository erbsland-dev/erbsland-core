// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PunycodeDecoder.hpp"

#include "impl/IdnaProcessor.hpp"
#include "impl/PunycodeCodec.hpp"

#include "../../err/ParseError.hpp"

#include <utility>

namespace erbsland::text::punycode {

PunycodeDecoder::PunycodeDecoder(String text, PunycodeOptions options) :
    _text{std::move(text)}, _options{std::move(options)} {
}

auto PunycodeDecoder::decode() const -> std::optional<String> {
    try {
        return decodeOrThrow();
    } catch (const err::ParseError &) {
        return {};
    }
}

auto PunycodeDecoder::decodeOrThrow() const -> String {
    if (_options.mode() == PunycodeMode::Pure) {
        return impl::decodePunycodePayload(_text);
    }
    return impl::IdnaProcessor{_options}.toUnicode(_text);
}

}
