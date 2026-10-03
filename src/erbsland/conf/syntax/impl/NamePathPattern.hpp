// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../text/StringPattern.hpp"
#include "../../NamePath.hpp"

#include <array>
#include <optional>

namespace erbsland::conf::syntax::impl {

/// A normalized name-path glob using component string patterns and recursive stars.
/// @tested{SyntaxFormatTest}
class NamePathPattern final {
    /// One name transition.
    /// @tested{SyntaxFormatTest}
    struct Component {
        std::optional<text::StringPattern> pattern; ///< Nonrecursive string pattern.
        std::optional<NameType> type;               ///< Literal names retain their name category.
        bool recursive{};                           ///< A whole-component recursive star.
        /// Match this component or complete pattern against a name path.
        auto matches(const Name &name) const -> bool;
    };

public:
    /// Compile a complete name-path selector.
    /// @param pattern Glob text with dots separating components.
    /// @throws err::ParseError If the pattern is malformed.
    explicit NamePathPattern(const text::String &pattern);
    /// Match a locally resolved path.
    /// @param path Candidate declaration path.
    /// @return Whether all path components match.
    /// Match this component or complete pattern against a name path.
    [[nodiscard]] auto matches(const NamePath &path) const -> bool;
    /// Get the path specificity tuple.
    [[nodiscard]] auto specificity() const noexcept -> std::array<int, 5> { return _specificity; }

private:
    /// Compile one name component and update specificity.
    void append(text::String component);

private:
    std::vector<Component> _components;             ///< Component patterns.
    std::array<int, 5> _specificity{2, 0, 0, 0, 0}; ///< Exactness, literal components/chars, negative wildcard counts.
};

}
