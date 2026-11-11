// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

namespace erbsland::conf::impl {

/// The purpose of a node in the rule tree, independent of its value type.
enum class RuleRole {
    Root,           ///< The implicit root for the document or validated section.
    NamedValue,     ///< A value selected by its literal configuration name.
    AnyValue,       ///< The fallback rule for values without a named rule.
    ListEntry,      ///< The shared rule for entries of a list or matrix.
    NameConstraint, ///< Constraints on the name of the parent's validated value.
    Alternative,    ///< One candidate beneath an alternatives container.
};

}
