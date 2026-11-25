// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ProjectAccessCheck.hpp"

#include <erbsland/conf/SourceIdentifier.hpp>

namespace tool::frame {

using namespace el::text::literals;

auto ProjectAccessCheck::check(const el::conf::AccessSources &sources) -> el::conf::AccessCheckResult {
    using el::conf::AccessCheckResult;
    if (!sources.source || sources.source->name() != "file"_el) {
        return AccessCheckResult::Denied;
    }
    const auto path = el::Path{sources.source->path()};
    if (!path.isAbsolute()) {
        return AccessCheckResult::Denied;
    }
    const auto isRelativeToProject = path.resolveOrThrow(el::PathResolveMode::Lexical).isRelativeTo(_projectRoot);
    if (isRelativeToProject) {
        return AccessCheckResult::Granted;
    }
    return AccessCheckResult::Denied;
}

}
