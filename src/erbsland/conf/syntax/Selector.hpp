// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Statement.hpp"

#include "impl/NamePathPattern.hpp"

#include <array>

namespace erbsland::conf::syntax {

/// Conditions selecting statements; conditions combine with AND and path alternatives with OR.
/// @tested{SyntaxFormatTest}
class Selector final {
public: // conditions
    /// Select the statement kind.
    auto setKind(Statement::Kind value) -> Selector &;
    /// Select list or map sections.
    auto setListSection(bool value) -> Selector &;
    /// Select relative or absolute sections.
    auto setRelativeSection(bool value) -> Selector &;
    /// Select headers declaring more than this many names.
    auto setMinimumSectionNames(std::size_t value) -> Selector &;
    /// Select meta or ordinary assignments.
    auto setMetaValue(bool value) -> Selector &;
    /// Select any of these recognized value types.
    auto setValueTypes(std::vector<ValueType> values) -> Selector &;
    /// Add an alternative normalized name-path glob.
    /// @param pattern Glob text; a whole-component ** crosses name boundaries.
    /// @throws err::ParseError If the pattern is malformed.
    auto addPath(const text::String &pattern) -> Selector &;

public: // matching
    /// Get matching specificity, or no value when any condition fails.
    /// @param statement Statement to inspect.
    /// @return Path specificity followed by the count of other conditions.
    [[nodiscard]] auto match(const Statement &statement) const -> std::optional<std::array<int, 6>>;

private:
    std::optional<Statement::Kind> _kind;      ///< Statement filter.
    std::optional<bool> _list;                 ///< List-section filter.
    std::optional<bool> _relative;             ///< Relative-section filter.
    std::optional<std::size_t> _names;         ///< Strict lower bound for declared names.
    std::optional<bool> _meta;                 ///< Meta-value filter.
    std::vector<ValueType> _types;             ///< Alternative value types.
    std::vector<impl::NamePathPattern> _paths; ///< Compiled alternative paths.
};

}
