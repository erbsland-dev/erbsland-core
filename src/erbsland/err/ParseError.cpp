// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ParseError.hpp"

#include "../text/StringFormat.hpp"

namespace erbsland::err {

ParseError::ParseError(text::StringView reason, unit::CpIndex position) noexcept :
    RuntimeError{std::move(reason)}, _position{position} {
}

ParseError::ParseError(const std::string_view reason, unit::CpIndex position) noexcept :
    ParseError{text::String{reason}, position} {
}

auto ParseError::toString() const noexcept -> text::StringView {
    if (!hasPosition()) {
        return reason();
    }
    static auto messageFormat = text::StringFormat{"{} at code point {}"};
    return messageFormat.build(reason(), _position.toSizeT());
}

auto ParseError::hasPosition() const noexcept -> bool {
    return !_position.isNoIndex();
}

auto ParseError::position() const noexcept -> unit::CpIndex {
    return _position;
}

}
