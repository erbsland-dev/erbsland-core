// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ReplacerErrorCategory.hpp"

#include "../../err/RuntimeError.hpp"
#include "../../unit/CpIndex.hpp"

#include <optional>
#include <utility>

namespace erbsland::text::placeholder {

/// An error while parsing or evaluating a placeholder.
/// The optional offset is a zero-based code-point index in the original input.
/// @tested{ReplacerTest ParserPlaceholderTest}
class ReplacerError final : public err::RuntimeError {
public:
    /// Create an error with a category, reason, and optional input offset.
    ReplacerError(ReplacerErrorCategory category, String reason, std::optional<unit::CpIndex> offset = {}) :
        err::RuntimeError{std::move(reason)}, _category{category}, _offset{offset} {}

    // defaults
    ~ReplacerError() override = default;

public:
    /// Get the error category.
    [[nodiscard]] auto category() const noexcept -> ReplacerErrorCategory { return _category; }
    /// Get the input offset, when available.
    [[nodiscard]] auto offset() const noexcept -> std::optional<unit::CpIndex> { return _offset; }
    /// Return an error with an input offset.
    [[nodiscard]] auto withOffset(unit::CpIndex offset) const -> ReplacerError {
        return ReplacerError{_category, reason(), offset};
    }

private:
    ReplacerErrorCategory _category;      ///< Failure category.
    std::optional<unit::CpIndex> _offset; ///< Original input offset, if known.
};

}
