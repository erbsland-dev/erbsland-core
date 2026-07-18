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

/// Root configuration object for command line options, global option sets, built-ins, and modules.
/// @tested{OptionsFrameworkTest OptionsParserTest}
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
    /// @return A shared options root with the built-in help and version options.
    [[nodiscard]] static auto create() -> OptionsPtr;
    /// Add a main option set.
    /// @param optionSet The global option set to activate for root parsing and every selected module.
    void addSet(OptionSetPtr optionSet);
    /// Add an option module.
    /// @param optionModule The module that can be selected as the first ordinary command-line argument.
    void addModule(OptionModulePtr optionModule);

public: // implement OptionsManager
    auto addOption(std::initializer_list<text::String> names) -> OptionEditor override;
    auto editOption(const text::String &name) -> OptionEditor override;

public: // accessors
    /// Get the help metadata for the options root.
    [[nodiscard]] auto help() const noexcept -> const OptionHelp & { return _help; }
    /// Set the complete help metadata for the options root.
    /// @param help The replacement help metadata. The description becomes the root help summary.
    void setHelp(OptionHelp help) { _help = std::move(help); }
    /// Set the help title for the options root.
    /// @param title Root title used by renderers that expose title text.
    void setHelpTitle(text::String title) { _help.setTitle(std::move(title)); }
    /// Set the help description for the options root.
    /// @param description Summary paragraph shown before generated root help.
    void setHelpDescription(text::String description) { _help.setDescription(std::move(description)); }
    /// Set the help epilog for the options root.
    /// @param epilog Text rendered after root help output.
    void setHelpEpilog(text::String epilog) { _help.setEpilog(std::move(epilog)); }
    /// Set the help visibility for the options root.
    /// @param visibility Root help visibility metadata for custom renderers.
    void setHelpVisibility(const OptionHelpVisibility visibility) noexcept { _help.setVisibility(visibility); }
    /// Get the unprocessed executable path from the command line.
    [[nodiscard]] auto executablePath() const noexcept -> const text::String & { return _executablePath; }
    /// Set the unprocessed executable path from the command line.
    /// @param executablePath The original `argv[0]` text. The executable name is extracted for usage output.
    void setExecutablePath(text::String executablePath);
    /// Get the extracted executable name.
    [[nodiscard]] auto executableName() const noexcept -> const text::String & { return _executableName; }
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
    text::String _executablePath;                ///< The unprocessed executable path from the command line.
    text::String _executableName;                ///< The extracted executable name.
    core::ApplicationInfo _applicationInfo;      ///< The application metadata.
    std::vector<OptionModulePtr> _optionModules; ///< The available option modules.
    OptionSetPtr _builtInOptionSet;              ///< The built-in option set.
    std::vector<OptionSetPtr> _optionSets;       ///< The main option sets.
};

}
