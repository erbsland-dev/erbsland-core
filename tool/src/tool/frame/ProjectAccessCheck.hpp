// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/conf/AccessCheck.hpp>
#include <erbsland/Path.hpp>

#include <utility>

namespace tool::frame {

/// Validate every configuration source before the parser opens it.
/// @tested{ProjectFilesTest}
class ProjectAccessCheck final : public el::conf::AccessCheck {
public:
    /// Create a configuration source guard.
    /// @param projectRoot Successfully validated absolute project root.
    explicit ProjectAccessCheck(el::Path projectRoot) : _projectRoot{std::move(projectRoot)} {}

public: // implement AccessCheck
    auto check(const el::conf::AccessSources &sources) -> el::conf::AccessCheckResult override;

private:
    el::Path _projectRoot; ///< Validated absolute project root.
};

}
