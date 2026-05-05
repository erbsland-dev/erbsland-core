// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "OptionEditor.hpp"

#include "../text/String.hpp"

#include <initializer_list>

namespace erbsland::options {

/// A common interface for objects that can own option definitions.
/// @tested{OptionsFrameworkTest}
class OptionSetManager {
public:
    OptionSetManager() = default;
    virtual ~OptionSetManager() = default;
    OptionSetManager(const OptionSetManager &) = default;
    auto operator=(const OptionSetManager &) -> OptionSetManager & = default;
    OptionSetManager(OptionSetManager &&) = default;
    auto operator=(OptionSetManager &&) -> OptionSetManager & = default;

public:
    /// Add an option with one name.
    virtual auto addOption(const text::StringView &name) -> OptionEditor;
    /// Add an option with one or more names.
    virtual auto addOption(std::initializer_list<text::StringView> names) -> OptionEditor = 0;
    /// Edit an existing option by name.
    virtual auto editOption(const text::StringView &name) -> OptionEditor = 0;
};

}
