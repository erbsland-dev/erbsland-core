// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "OptionDisplayRow.hpp"

#include "../Option_fwd.hpp"
#include "../OptionDisplayText.hpp"
#include "../OptionHelp.hpp"
#include "../OptionModule_fwd.hpp"
#include "../Options_fwd.hpp"
#include "../OptionSet_fwd.hpp"

#include "../../text/String.hpp"
#include "../../text/StringView.hpp"

#include <vector>

namespace erbsland::options::impl {

/// Shared model for option help, version and error rendering.
/// @tested{StandardOptionRendererTest TerminalOptionsRendererTest}
class OptionDisplayModel final {
public:
    /// Create a display model for the given options and module name.
    OptionDisplayModel(
        OptionsPtr options,
        text::StringView moduleName,
        OptionDisplayText displayText = OptionDisplayText::defaultText());

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
    [[nodiscard]] auto displayText() const noexcept -> const OptionDisplayText & { return _displayText; }
    /// Get the application display name.
    [[nodiscard]] auto displayName() const -> text::StringView;
    /// Get the executable name for usage text.
    [[nodiscard]] auto executableName() const -> text::StringView;
    /// Get the first help heading.
    [[nodiscard]] auto helpTitleText() const -> text::String;
    /// Get the selected title text.
    [[nodiscard]] auto titleText() const -> text::StringView;
    /// Get the selected help object.
    [[nodiscard]] auto selectedHelp() const -> const OptionHelp *;
    /// Get the visible option sets for the selected scope.
    [[nodiscard]] auto visibleOptionSets() const -> std::vector<OptionSetPtr>;
    /// Get display rows for options in the selected scope.
    [[nodiscard]] auto optionRows() const -> std::vector<OptionDisplayRow>;
    /// Get display rows for modules.
    [[nodiscard]] auto moduleRows() const -> std::vector<OptionDisplayRow>;
    /// Create the display title for one option.
    [[nodiscard]] static auto optionTitle(
        const OptionPtr &option, const OptionDisplayText &displayText = OptionDisplayText::defaultText())
        -> text::String;
    /// Create the display description for one option.
    [[nodiscard]] static auto optionDescription(
        const OptionPtr &option, const OptionDisplayText &displayText = OptionDisplayText::defaultText())
        -> text::String;
    /// Test if a help block is visible in normal help output.
    [[nodiscard]] static auto visibleHelp(const OptionHelp &help) noexcept -> bool;
    /// Test if this option is the built-in help option.
    [[nodiscard]] static auto isHelpOption(const OptionPtr &option) -> bool;
    /// Test if this option is the built-in version option.
    [[nodiscard]] static auto isVersionOption(const OptionPtr &option) -> bool;

private:
    [[nodiscard]] auto findModule(text::StringView moduleName) const -> OptionModulePtr;
    [[nodiscard]] auto visibleOptionSet(const OptionSetPtr &optionSet) const noexcept -> bool;
    [[nodiscard]] auto choiceRows(const OptionPtr &option) const -> std::vector<OptionDisplayRow>;

private:
    OptionsPtr _options;            ///< The options root to render.
    OptionModulePtr _module;        ///< The selected module, if any.
    OptionDisplayText _displayText; ///< Display text used by this model.
};

}
