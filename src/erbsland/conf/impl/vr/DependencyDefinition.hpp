// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "DependencyMode.hpp"

#include "../../Location.hpp"
#include "../../NamePath.hpp"

#include <memory>

namespace erbsland::conf::impl {

class DependencyDefinition;
using DependencyDefinitionPtr = std::shared_ptr<DependencyDefinition>;
using DependencyDefinitionList = std::vector<DependencyDefinitionPtr>;

/// A dependency definition.
class DependencyDefinition {
public:
    /// Create a new dependency definition.
    /// @param mode The dependency mode.
    /// @param sources The source paths.
    /// @param targets The target paths.
    /// @param errorMessage An optional custom error message. Empty for no custom message.
    DependencyDefinition(
        DependencyMode mode, NamePathList sources, NamePathList targets, text::String errorMessage) noexcept;
    virtual ~DependencyDefinition() = default;

    /// Create a new dependency definition.
    /// @param mode The dependency mode.
    /// @param sources The source paths.
    /// @param targets The target paths.
    /// @param errorMessage An optional custom error message. Empty for no custom message.
    [[nodiscard]] static auto create(
        DependencyMode mode, NamePathList sources, NamePathList targets, text::String errorMessage)
        -> DependencyDefinitionPtr;

public:
    /// The dependency mode.
    [[nodiscard]] auto mode() const noexcept -> DependencyMode { return _mode; }
    /// The source values.
    [[nodiscard]] auto sources() const noexcept -> const NamePathList & { return _sources; }
    /// The target values.
    [[nodiscard]] auto targets() const noexcept -> const NamePathList & { return _targets; }
    /// Test if there is a custom error message.
    [[nodiscard]] auto hasErrorMessage() const noexcept -> bool { return !_errorMessage.isEmpty(); }
    /// A custom error message.
    [[nodiscard]] auto errorMessage() const noexcept -> const text::String & { return _errorMessage; }
    /// The location of the dependency definition in the source file.
    [[nodiscard]] auto location() const noexcept -> const Location & { return _location; }
    /// Set the location of the dependency definition.
    void setLocation(const Location &location) noexcept { _location = location; }

private:
    DependencyMode _mode{DependencyMode::If}; ///< The dependency mode.
    NamePathList _sources;                    ///< The source values.
    NamePathList _targets;                    ///< The target values.
    text::String _errorMessage;               ///< A custom error message.
    Location _location;                       ///< The location of the dependency definition in the source file.
};

}
