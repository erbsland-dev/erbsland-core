// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../text/StringView.hpp"
#include "../../unit/ByteIndex.hpp"

namespace erbsland::path::impl {

/// Find the first suffix separator in a file name.
[[nodiscard]] auto firstSuffixPosition(const text::StringView &name) noexcept -> unit::ByteIndex;

/// Find the last suffix separator in a file name.
[[nodiscard]] auto lastSuffixPosition(const text::StringView &name) noexcept -> unit::ByteIndex;

/// Return the last suffix of a file name.
[[nodiscard]] auto lastSuffix(const text::StringView &name) noexcept -> text::StringView;

/// Return all suffixes of a file name.
[[nodiscard]] auto suffixes(const text::StringView &name) noexcept -> text::StringView;

/// Return the stem of a file name.
[[nodiscard]] auto stem(const text::StringView &name) noexcept -> text::StringView;

/// Normalize a suffix replacement.
[[nodiscard]] auto normalizedSuffixReplacement(const text::StringView &replacement) -> text::StringView;

}
