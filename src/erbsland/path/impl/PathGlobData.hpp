// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../Path.hpp"
#include "../PathGlobMatchMode.hpp"

#include "../../text/impl/StringPatternData_fwd.hpp"
#include "../../unit/CpIndex.hpp"

#include <vector>

namespace erbsland::path::impl {

/// Immutable path-glob program and its shared matching operations.
/// @tested{PathGlobPatternTest}
class PathGlobData final {
public:
    /// One component transition in the path program.
    /// @tested{PathGlobPatternTest}
    struct Component {
        /// Match a nonrecursive component using its compiled program or decoded literal.
        [[nodiscard]] auto matches(const text::String &text) const -> bool;

        text::impl::StringPatternDataPtr pattern; ///< Non-null for a nonliteral component pattern.
        text::String literal;   ///< Decoded literal component, empty for patterns and recursive stars.
        bool recursive{};       ///< Whether the transition consumes zero or more complete components.
        unit::CpIndex position; ///< Source position for consumer diagnostics.
    };
    /// One alternative, including root information retained for filesystem consumers.
    /// @tested{PathGlobPatternTest}
    struct Alternative {
        std::vector<Component> components; ///< Ordered component transitions.
        text::String root;                 ///< Literal root, ignored by matching but retained for include resolution.
        bool anchored{};                   ///< Whether suffix matching is anchored at the first component.
        bool hasRecursiveWildcard{};       ///< Whether matching requires recursive component transitions.
    };

public: // matching
    /// Match or test strict-descendant feasibility using local reusable working storage.
    /// @param path Candidate path.
    /// @param mode Matching mode.
    /// @param descendant Whether to test future strict descendants instead of the candidate itself.
    /// @return Whether any alternative accepts the requested condition.
    [[nodiscard]] auto match(const Path &path, PathGlobMatchMode mode, bool descendant) const -> bool;

public:
    std::vector<Alternative> alternatives; ///< Compiled OR alternatives.
};

}
