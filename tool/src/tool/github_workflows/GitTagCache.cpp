// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "GitTagCache.hpp"

#include <erbsland/ApplicationError.hpp>
#include <erbsland/StringJoins.hpp>

namespace tool::github_workflows {

using namespace el::text::literals;

auto GitTagCache::get(const el::String &repository) -> std::shared_ptr<const TagCatalog> {
    if (_catalogs.contains(repository)) {
        return _catalogs.get(repository, {});
    }
    const auto result = _git.run(
        el::StringList{"ls-remote"_el, "--tags"_el, el::join("https://github.com/"_el, repository, ".git"_el)},
        _directory);
    if (!result.status.isSuccess()) {
        throw el::ApplicationError{
            "Git Tag Lookup Failed"_el,
            el::join("Could not verify stable tags for "_el, repository, ". "_el, result.error)};
    }
    auto catalog = std::make_shared<TagCatalog>(result.output, repository);
    _catalogs.set(repository, catalog);
    return catalog;
}

}
