// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TagCatalog.hpp"

#include "../frame/ExternalCommand.hpp"

#include <memory>

namespace tool::github_workflows {

/// Retain one validated tag response per repository for an entire command invocation.
class GitTagCache final {
public:
    /// Bind the prevalidated Git command and its working directory.
    GitTagCache(const frame::ExternalCommand &git, el::Path directory) : _git{git}, _directory{std::move(directory)} {}

public: // queries
    /// Load a repository once and retain its catalog.
    /// @param repository Validated owner/repository action prefix.
    /// @return Shared immutable catalog for this invocation.
    /// @throws el::ApplicationError If Git fails or its response is invalid.
    [[nodiscard]] auto get(const el::String &repository) -> std::shared_ptr<const TagCatalog>;

private:
    const frame::ExternalCommand &_git;                             ///< Command owned by the active workflow tool.
    el::Path _directory;                                            ///< Project working directory.
    el::StringHashMap<std::shared_ptr<const TagCatalog>> _catalogs; ///< Successfully loaded repositories.
};

}
