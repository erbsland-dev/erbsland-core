// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../text/String_fwd.hpp"
#include "../../../text/StringLiteral.hpp"

namespace erbsland::network::impl {

/// Test if a host name, IP address or IP endpoint text is valid.
/// - It must be not empty
/// - It must be smaller than 1024 bytes
/// - It must be valid UTF-8
/// @throws err::ParseError if the text is invalid
void testCommonHostText(const text::String &text, const text::StringLiteral &parsedObject);

}
