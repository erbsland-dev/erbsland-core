// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "OptionCallback.hpp"
#include "OptionHelp.hpp"
#include "OptionModule_fwd.hpp"
#include "OptionSet_fwd.hpp"
#include "OptionSetManager.hpp"

#include <utility>
#include <vector>

namespace erbsland::options {

/// A command line module with its own option sets, callbacks, help metadata, and optional main function.
/// Modules are selected by the first ordinary command-line argument when an `Options` root has modules.
/// @tested{OptionsFrameworkTest}
class OptionModule : public OptionSetManager {
public:
    /// Create an empty option module.
    OptionModule() = default;
    /// Create a module with a command line name.
    explicit OptionModule(const text::String &name);

    // defaults
    ~OptionModule() override = default;
    OptionModule(const OptionModule &) = default;
    auto operator=(const OptionModule &) -> OptionModule & = default;
    OptionModule(OptionModule &&) = default;
    auto operator=(OptionModule &&) -> OptionModule & = default;

public:
    using OptionSetManager::addOption;

    /// Create an empty shared module.
    /// @return A shared module without a command-line name.
    [[nodiscard]] static auto create() -> OptionModulePtr;
    /// Create a shared module with a command line name.
    /// @param name The module selector accepted on the command line.
    /// @return A shared module with the given selector.
    [[nodiscard]] static auto create(const text::String &name) -> OptionModulePtr;
    /// Test if a module name is valid.
    /// @param name The module selector to validate.
    /// @return `true` if the name can be used as a module selector.
    [[nodiscard]] static auto isValidName(const text::String &name) noexcept -> bool;
    /// Add an option set to this module.
    /// @param optionSet The set that becomes active when this module is selected.
    void addSet(OptionSetPtr optionSet);

public: // implement OptionsManager
    /// Add an option with one or more names.
    auto addOption(std::initializer_list<text::String> names) -> OptionEditor override;
    /// Edit an option selected by one of its names.
    auto editOption(const text::String &name) -> OptionEditor override;

public: // accessors
    /// Get the command line module name.
    [[nodiscard]] auto name() const noexcept -> const text::String & { return _name; }
    /// Set the command line module name.
    /// @param name The selector accepted as the first ordinary argument.
    void setName(const text::String &name);
    /// Test if the given name matches this module.
    [[nodiscard]] auto hasName(const text::String &name) const -> bool;
    /// Get the help metadata for this module.
    [[nodiscard]] auto help() const noexcept -> const OptionHelp & { return _help; }
    /// Set the complete help metadata for this module.
    /// @param help The replacement help metadata. The description is shown in the root module list.
    void setHelp(OptionHelp help) { _help = std::move(help); }
    /// Set the help title for this module.
    /// @param title Short module title used when no description is available.
    void setHelpTitle(text::String title) { _help.setTitle(std::move(title)); }
    /// Set the help description for this module.
    /// @param description Description shown for this module and as module-help summary.
    void setHelpDescription(text::String description) { _help.setDescription(std::move(description)); }
    /// Set the help epilog for this module.
    /// @param epilog Text rendered after module-specific help output.
    void setHelpEpilog(text::String epilog) { _help.setEpilog(std::move(epilog)); }
    /// Set the help visibility for this module.
    /// @param visibility Controls whether this module appears in root help.
    void setHelpVisibility(const OptionHelpVisibility visibility) noexcept { _help.setVisibility(visibility); }
    /// Get all option sets.
    [[nodiscard]] auto optionSets() const noexcept -> const std::vector<OptionSetPtr> & { return _optionSets; }
    /// Get the pre-parsing callback.
    [[nodiscard]] auto preParsingFn() const noexcept -> const PreOptionModuleParsingFn & { return _preParsingFn; }
    /// Set the pre-parsing callback.
    void setPreParsingFn(PreOptionModuleParsingFn fn) { _preParsingFn = std::move(fn); }
    /// Get the post-parsing callback.
    [[nodiscard]] auto postParsingFn() const noexcept -> const PostParsingFn & { return _postParsingFn; }
    /// Set the post-parsing callback.
    void setPostParsingFn(PostParsingFn fn) { _postParsingFn = std::move(fn); }
    /// Get the module main function.
    [[nodiscard]] auto mainFn() const noexcept -> const ModuleMainFn & { return _mainFn; }
    /// Set the module main function.
    /// @param fn The function called by `Application` after this module is parsed successfully.
    void setMainFn(ModuleMainFn fn) { _mainFn = std::move(fn); }

private:
    /// Get or create the option set used for direct module options.
    [[nodiscard]] auto defaultOptionSet() -> OptionSetPtr;

private:
    text::String _name;                     ///< The command line module name.
    OptionHelp _help;                       ///< The help text for the module.
    std::vector<OptionSetPtr> _optionSets;  ///< The option sets for this module.
    PreOptionModuleParsingFn _preParsingFn; ///< Called before parsing starts.
    PostParsingFn _postParsingFn;           ///< Called after successful parsing.
    ModuleMainFn _mainFn;                   ///< The main function if this module is selected.
};

}
