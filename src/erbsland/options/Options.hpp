// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "OptionHelp.hpp"
#include "OptionModule_fwd.hpp"
#include "Options_fwd.hpp"
#include "OptionSet_fwd.hpp"
#include "OptionSetManager.hpp"

#include "../core/ApplicationInfo.hpp"

#include <vector>

namespace erbsland::options {

/// Root configuration object for command line options and modules.
/// @tested{OptionsFrameworkTest, OptionsParserTest}
class Options : public OptionSetManager {
public:
    Options();

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
    /// Get the unprocessed executable path from the command line.
    [[nodiscard]] auto executablePath() const noexcept -> const text::StringView & { return _executablePath; }
    /// Set the unprocessed executable path from the command line.
    void setExecutablePath(text::StringView executablePath);
    /// Get the extracted executable name.
    [[nodiscard]] auto executableName() const noexcept -> const text::StringView & { return _executableName; }
    /// Get the application metadata.
    [[nodiscard]] auto applicationInfo() const noexcept -> const core::ApplicationInfo & { return _applicationInfo; }
    /// Set the application metadata.
    void setApplicationInfo(core::ApplicationInfo applicationInfo) { _applicationInfo = std::move(applicationInfo); }
    /// Get all option modules.
    [[nodiscard]] auto optionModules() const noexcept -> const std::vector<OptionModulePtr> & { return _optionModules; }
    /// Get the built-in option set.
    [[nodiscard]] auto builtInOptionSet() const noexcept -> const OptionSetPtr & { return _builtInOptionSet; }
    /// Get all main option sets.
    [[nodiscard]] auto optionSets() const noexcept -> const std::vector<OptionSetPtr> & { return _optionSets; }

private:
    [[nodiscard]] auto defaultOptionSet() -> OptionSetPtr;
    [[nodiscard]] static auto createBuiltInOptionSet() -> OptionSetPtr;

private:
    OptionHelp _help;                            ///< The help text for the options root.
    text::StringView _executablePath;            ///< The unprocessed executable path from the command line.
    text::StringView _executableName;            ///< The extracted executable name.
    core::ApplicationInfo _applicationInfo;      ///< The application metadata.
    std::vector<OptionModulePtr> _optionModules; ///< The available option modules.
    OptionSetPtr _builtInOptionSet;              ///< The built-in option set.
    std::vector<OptionSetPtr> _optionSets;       ///< The main option sets.
};

}
