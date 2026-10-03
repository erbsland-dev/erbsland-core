// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Severity.hpp"

#include <erbsland/ByteRange.hpp>
#include <erbsland/LineIndex.hpp>
#include <erbsland/LineRange.hpp>
#include <erbsland/Path.hpp>
#include <erbsland/String.hpp>

namespace tool::anti_patterns {

/// One source finding with stable report coordinates and optional acceptance.
struct Finding final {
    el::String rule;         ///< Stable rule identifier.
    Severity severity{};     ///< Report priority.
    el::Path path;           ///< Project-relative source path.
    el::ByteRange range;     ///< UTF-8 source span, with an exclusive end.
    el::LineRange lines;     ///< Zero-based source lines, with an exclusive end.
    el::String snippet;      ///< Compact source excerpt.
    el::String inlineReason; ///< Adjacent source acceptance reason, if any.
    el::String configReason; ///< Configured acceptance reason, if any.

    /// Test whether this finding has an accepted exception.
    [[nodiscard]] auto isSuppressed() const noexcept -> bool {
        return !inlineReason.isEmpty() || !configReason.isEmpty();
    }

    /// Get the one-based first line for reports.
    [[nodiscard]] auto lineNumber() const noexcept -> el::LineIndex::Value {
        return lines.beginIndex().toRawValue() + 1;
    }
    /// Get the one-based final line for reports.
    [[nodiscard]] auto endLineNumber() const noexcept -> el::LineIndex::Value { return lines.endIndex().toRawValue(); }
};

}
