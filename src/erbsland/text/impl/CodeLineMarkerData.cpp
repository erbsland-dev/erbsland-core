// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "CodeLineMarkerData.hpp"

#include "../Literals.hpp"
#include "../StringBuilder.hpp"

namespace erbsland::text::impl {

using namespace literals;

auto CodeLineMarkerData::toString() const -> StringView {
    if (_text.isEmpty()) {
        auto builder = StringBuilder{};
        builder.append(String::fromInteger(_range.index().toSizeT()));
        builder.append(":"_el);
        builder.append(String::fromInteger(_range.length().toSizeT()));
        _text = builder.toString();
    }
    return _text;
}

}
