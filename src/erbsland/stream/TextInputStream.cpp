// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TextInputStream.hpp"

namespace erbsland::stream {

auto TextInputStream::read() -> std::optional<text::String> {
    return read(cDefaultTextReadMaximum);
}

auto TextInputStream::readLine() -> std::optional<text::String> {
    return readLine(cDefaultTextReadMaximum);
}

auto TextInputStream::readAll() -> text::String {
    return readAll(cDefaultTextReadMaximum);
}

}
