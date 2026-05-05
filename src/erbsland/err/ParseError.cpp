// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ParseError.hpp"

#include "../text/StringFormat.hpp"

namespace erbsland::err {

auto ParseError::toString() const noexcept -> text::StringView {
    if (!hasPosition()) {
        return reason();
    }
    static auto messageFormat = text::StringFormat{"{} at code point {}"};
    return messageFormat.build(reason(), _position.toSizeT());
}

}
