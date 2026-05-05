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

/// A command line module with its own option sets and optional main function.
/// @tested{OptionsFrameworkTest}
class OptionModule : public OptionSetManager {
public:
    OptionModule() = default;
    /// Create a module with a command line name.
    explicit OptionModule(text::StringView name);

    // defaults
    ~OptionModule() override = default;
    OptionModule(const OptionModule &) = default;
    auto operator=(const OptionModule &) -> OptionModule & = default;
    OptionModule(OptionModule &&) = default;
    auto operator=(OptionModule &&) -> OptionModule & = default;

public:
    using OptionSetManager::addOption;

    /// Create an empty shared module.
    [[nodiscard]] static auto create() -> OptionModulePtr;
    /// Create a shared module with a command line name.
    [[nodiscard]] static auto create(text::StringView name) -> OptionModulePtr;
    /// Test if a module name is valid.
    [[nodiscard]] static auto isValidName(const text::StringView &name) noexcept -> bool;
    /// Add an option set to this module.
    void addSet(OptionSetPtr optionSet);

public: // implement OptionsManager
    auto addOption(std::initializer_list<text::StringView> names) -> OptionEditor override;
    auto editOption(const text::StringView &name) -> OptionEditor override;

public: // accessors
    /// Get the command line module name.
    [[nodiscard]] auto name() const noexcept -> const text::StringView & { return _name; }
    /// Set the command line module name.
    void setName(text::StringView name);
    /// Test if the given name matches this module.
    [[nodiscard]] auto hasName(const text::StringView &name) const -> bool;
    /// Get the help text.
    [[nodiscard]] auto help() const noexcept -> const OptionHelp & { return _help; }
    /// Set the help text.
    void setHelp(OptionHelp help) { _help = std::move(help); }
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
    void setMainFn(ModuleMainFn fn) { _mainFn = std::move(fn); }

private:
    [[nodiscard]] auto defaultOptionSet() -> OptionSetPtr;

private:
    text::StringView _name;                 ///< The command line module name.
    OptionHelp _help;                       ///< The help text for the module.
    std::vector<OptionSetPtr> _optionSets;  ///< The option sets for this module.
    PreOptionModuleParsingFn _preParsingFn; ///< Called before parsing starts.
    PostParsingFn _postParsingFn;           ///< Called after successful parsing.
    ModuleMainFn _mainFn;                   ///< The main function if this module is selected.
};

}
