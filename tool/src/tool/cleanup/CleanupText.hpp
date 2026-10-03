// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/String.hpp>
#include <erbsland/StringList.hpp>

namespace tool::cleanup::text {

/// Split physical lines, removing CRLF terminators and the final empty split.
/// @param text Source text.
/// @return Physical lines without terminators.
[[nodiscard]] auto lines(const el::String &text) -> el::StringList;
/// Join physical lines with LF and a final terminator.
/// @param lines Physical lines without terminators.
/// @return Complete text.
[[nodiscard]] auto join(const el::StringList &lines) -> el::String;
/// Calculate the persistent SHA-256 fingerprint of exact text bytes.
/// @param text Text to hash.
/// @return Printable digest.
[[nodiscard]] auto digest(const el::String &text) -> el::String;
/// Raise a contextual operational error.
/// @param description Error explanation.
/// @param path Optional source path.
[[noreturn]] void fail(const el::String &description, const el::String &path = {});

}
