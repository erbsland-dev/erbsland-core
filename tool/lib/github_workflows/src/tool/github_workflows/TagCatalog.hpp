// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ActionTag.hpp"

#include <erbsland/StringHashMap.hpp>

#include <optional>

namespace tool::github_workflows {

/// Validated stable tag catalog from a single git ls-remote response.
class TagCatalog final {
public:
    /// Parse remote tags, preferring peeled annotated targets independent of row order.
    /// @param text Complete git ls-remote --tags output.
    /// @param repository Owner/repository display name.
    /// @throws el::ApplicationError If the response has malformed or conflicting rows.
    TagCatalog(const el::String &text, const el::String &repository);

public: // queries
    /// Get an exact tag spelling, or no value when absent.
    [[nodiscard]] auto named(const el::String &name) const -> std::optional<ActionTag>;
    /// Get the highest stable tag, or no value when the repository has none.
    [[nodiscard]] auto latest() const -> std::optional<ActionTag>;
    /// Get the highest stable tag for a commit SHA, or no value when absent.
    [[nodiscard]] auto forSha(const el::String &sha) const -> std::optional<ActionTag>;

private:
    /// Select a numerically higher tag, breaking aliases deterministically by exact spelling.
    [[nodiscard]] static auto higher(const ActionTag &left, const ActionTag &right) -> bool;

private:
    el::StringHashMap<ActionTag> _tags; ///< Stable tags by exact remote spelling.
};

}
