// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "OptionDisplayInfo.hpp"
#include "OptionHelp.hpp"
#include "OptionModule_fwd.hpp"
#include "Options_fwd.hpp"
#include "OptionSet_fwd.hpp"
#include "OptionSetManager.hpp"

#include <vector>

namespace erbsland::options {

/// Root configuration object for command line options and modules.
/// @tested{OptionsFrameworkTest}
class Options : public OptionSetManager {
public:
    Options() = default;

    // defaults
    ~Options() override = default;
    Options(const Options &) = default;
    auto operator=(const Options &) -> Options & = default;
    Options(Options &&) = default;
    auto operator=(Options &&) -> Options & = default;

public:
    using OptionSetManager::addOption;

    /// Create an empty shared options root.
    [[nodiscard]] static auto create() -> OptionsPtr;
    /// Add a main option set.
    void addSet(OptionSetPtr optionSet);
    /// Add an option module.
    void addModule(OptionModulePtr optionModule);

public: // implement OptionsManager
    auto addOption(std::initializer_list<text::StringView> names) -> OptionEditor override;
    auto editOption(const text::StringView &name) -> OptionEditor override;

public: // accessors
    /// Get the help text for the options root.
    [[nodiscard]] auto help() const noexcept -> const OptionHelp & { return _help; }
    /// Set the help text for the options root.
    void setHelp(OptionHelp help) { _help = std::move(help); }
    /// Get the display metadata.
    [[nodiscard]] auto displayInfo() const noexcept -> const OptionDisplayInfo & { return _displayInfo; }
    /// Set the display metadata.
    void setDisplayInfo(OptionDisplayInfo displayInfo) { _displayInfo = std::move(displayInfo); }
    /// Get all option modules.
    [[nodiscard]] auto optionModules() const noexcept -> const std::vector<OptionModulePtr> & { return _optionModules; }
    /// Get all main option sets.
    [[nodiscard]] auto optionSets() const noexcept -> const std::vector<OptionSetPtr> & { return _optionSets; }

private:
    [[nodiscard]] auto defaultOptionSet() -> OptionSetPtr;

private:
    OptionHelp _help;                            ///< The help text for the options root.
    OptionDisplayInfo _displayInfo;              ///< The display metadata.
    std::vector<OptionModulePtr> _optionModules; ///< The available option modules.
    std::vector<OptionSetPtr> _optionSets;       ///< The main option sets.
};

}
