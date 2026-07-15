// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::text::html::impl {

/// The HTML token type.
enum class HtmlTokenType : uint8_t {
    End,      ///< Sentinel token that marks the end of the token stream.
    Text,     ///< Plain text content.
    TagOpen,  ///< An opening tag with name and attributes.
    TagClose, ///< A closing tag name like `strong`.
    Comment,  ///< A comment body without the `<!--` and `-->` markers.
    DocType,  ///< A doctype declaration without the `<!DOCTYPE` prefix.
};

}
