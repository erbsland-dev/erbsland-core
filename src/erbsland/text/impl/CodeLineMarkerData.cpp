// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "CodeLineMarkerData.hpp"

#include "../Literals.hpp"
#include "../StringEditor.hpp"

namespace erbsland::text::impl {

using namespace literals;

auto CodeLineMarkerData::toString() const -> String {
    if (_text.isEmpty()) {
        _text = String::fromJoined(
            {String::fromInteger(_range.index().toSizeT()), ":"_el, String::fromInteger(_range.length().toSizeT())});
    }
    return _text;
}

}
