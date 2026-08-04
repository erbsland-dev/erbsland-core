// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "OptionDisplayGroup.hpp"
#include "OptionDisplayModel_fwd.hpp"
#include "OptionDisplayRow.hpp"

#include "../Option_fwd.hpp"
#include "../OptionHelp.hpp"
#include "../OptionModule_fwd.hpp"
#include "../Options_fwd.hpp"
#include "../OptionSet_fwd.hpp"

#include "../../i18n/DisplayTextMap_fwd.hpp"
#include "../../text/String.hpp"
#include "../../text/StringEditor.hpp"

#include <vector>

namespace erbsland::options::impl {

/// Shared model for option help, version and error rendering.
/// @tested{OptionDocumentTest}
class OptionDisplayModel final {
public:
    /// Create a display model for the given options and module name.
    OptionDisplayModel(
        OptionsPtr options, const text::String &moduleName, const i18n::DisplayTextMapConstPtr &displayText = {});

    // defaults
    ~OptionDisplayModel() = default;
    OptionDisplayModel(const OptionDisplayModel &) = default;
    auto operator=(const OptionDisplayModel &) -> OptionDisplayModel & = default;
    OptionDisplayModel(OptionDisplayModel &&) = default;
    auto operator=(OptionDisplayModel &&) -> OptionDisplayModel & = default;

public:
    /// Access the selected options root.
    [[nodiscard]] auto options() const noexcept -> const OptionsPtr & { return _options; }
    /// Access the selected module, if any.
    [[nodiscard]] auto module() const noexcept -> const OptionModulePtr & { return _module; }
    /// Access the display text used by the model.
    [[nodiscard]] auto displayText() const noexcept -> const i18n::DisplayTextMapConstPtr & { return _displayText; }
    /// Get the application display name.
    [[nodiscard]] auto displayName() const -> text::String;
    /// Get the executable name for usage text.
    [[nodiscard]] auto executableName() const -> text::String;
    /// Get the first help heading.
    [[nodiscard]] auto helpTitleText() const -> text::String;
    /// Get the selected title text.
    [[nodiscard]] auto titleText() const -> text::String;
    /// Get the selected help object.
    [[nodiscard]] auto selectedHelp() const -> const OptionHelp *;
    /// Get the visible option sets for the selected scope.
    [[nodiscard]] auto visibleOptionSets() const -> std::vector<OptionSetPtr>;
    /// Get display rows for options in the selected scope.
    [[nodiscard]] auto optionRows() const -> std::vector<OptionDisplayRow>;
    /// Get grouped display rows for options in the selected scope.
    [[nodiscard]] auto optionGroups() const -> std::vector<OptionDisplayGroup>;
    /// Get display rows for modules.
    [[nodiscard]] auto moduleRows() const -> std::vector<OptionDisplayRow>;
    /// Get options that must be displayed explicitly in the usage line.
    [[nodiscard]] auto usageOptions() const -> std::vector<OptionPtr>;
    /// Get positional arguments displayed in the usage line.
    [[nodiscard]] auto usagePositionalOptions() const -> std::vector<OptionPtr>;
    /// Create the display title for one option.
    [[nodiscard]] static auto optionTitle(const OptionPtr &option, const i18n::DisplayTextMapConstPtr &displayText = {})
        -> text::String;
    /// Create the display description for one option.
    [[nodiscard]] static auto optionDescription(
        const OptionPtr &option, const i18n::DisplayTextMapConstPtr &displayText = {}) -> text::String;
    /// Create inline detail text for one option.
    [[nodiscard]] static auto optionDetails(
        const OptionPtr &option, const i18n::DisplayTextMapConstPtr &displayText = {}) -> text::String;
    /// Resolve the displayed value name for one option.
    [[nodiscard]] static auto optionValueName(
        const OptionPtr &option, const i18n::DisplayTextMapConstPtr &displayText = {}) -> text::String;
    /// Resolve the displayed value name for one positional argument.
    [[nodiscard]] static auto positionalValueName(
        const OptionPtr &option, const i18n::DisplayTextMapConstPtr &displayText = {}) -> text::String;
    /// Resolve inherited help visibility.
    [[nodiscard]] static auto resolvedVisibility(
        const OptionHelp &help, OptionHelpVisibility inherited = OptionHelpVisibility::Normal) noexcept
        -> OptionHelpVisibility;
    /// Test if a help block is visible in ordinary help output.
    [[nodiscard]] static auto visibleHelp(const OptionHelp &help) noexcept -> bool;
    /// Test if this option is the built-in help option.
    [[nodiscard]] static auto isHelpOption(const OptionPtr &option) -> bool;
    /// Test if this option is the built-in version option.
    [[nodiscard]] static auto isVersionOption(const OptionPtr &option) -> bool;

private:
    /// Append a formatted meta placeholder to text being built.
    static void appendMetaPlaceholder(text::StringEditor &result, const text::String &placeholder);
    /// Get the display title for an option set.
    [[nodiscard]] static auto optionSetTitle(
        const OptionSetPtr &optionSet, const i18n::DisplayTextMapConstPtr &displayText) -> text::String;

    /// Find a module by its command-line name.
    [[nodiscard]] auto findModule(const text::String &moduleName) const -> OptionModulePtr;
    /// Test whether the options root defines modules.
    [[nodiscard]] auto hasModules() const noexcept -> bool;
    /// Test whether an option set is visible in the selected scope.
    [[nodiscard]] auto visibleOptionSet(const OptionSetPtr &optionSet) const noexcept -> bool;
    /// Test whether an option is visible in the selected output context.
    [[nodiscard]] auto visibleOption(
        const OptionPtr &option, OptionHelpVisibility setVisibility, bool forUsage = false) const noexcept -> bool;
    /// Test whether an option is an enabled built-in option.
    [[nodiscard]] auto enabledBuiltInOption(const OptionSetPtr &optionSet, const OptionPtr &option) const noexcept
        -> bool;
    /// Test whether a choice is visible for its inherited help visibility.
    [[nodiscard]] auto visibleChoice(const OptionHelp &help, OptionHelpVisibility optionVisibility) const noexcept
        -> bool;
    /// Test whether a module is visible in root help.
    [[nodiscard]] auto visibleModule(const OptionHelp &help) const noexcept -> bool;
    /// Create display rows for an option's choices.
    [[nodiscard]] auto choiceRows(const OptionPtr &option) const -> std::vector<OptionDisplayRow>;
    /// Create a sortable display key for an option.
    [[nodiscard]] auto optionSortKey(const OptionPtr &option) const -> text::String;
    /// Resolve custom display text or the default wording.
    [[nodiscard]] static auto resolveDisplayText(const i18n::DisplayTextMapConstPtr &displayText) noexcept
        -> i18n::DisplayTextMapConstPtr;

private:
    OptionsPtr _options;                       ///< The options root to render.
    OptionModulePtr _module;                   ///< The selected module, if any.
    i18n::DisplayTextMapConstPtr _displayText; ///< Display text used by this model.
};

}
