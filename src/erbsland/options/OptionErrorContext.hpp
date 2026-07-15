// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Option_fwd.hpp"
#include "OptionErrorReason.hpp"
#include "OptionModule_fwd.hpp"
#include "Options_fwd.hpp"
#include "OptionSet_fwd.hpp"

#include "../i18n/DisplayTextMap_fwd.hpp"
#include "../text/StringView.hpp"
#include "../text/StringViewList.hpp"
#include "../unit/ArgumentUnit.hpp"

#include <utility>

namespace erbsland::options {

/// A detailed error context to pin the source and reason for an error.
/// @tested{OptionsFrameworkTest}
class OptionErrorContext {
public:
    // defaults
    OptionErrorContext() = default;
    ~OptionErrorContext() = default;
    OptionErrorContext(const OptionErrorContext &) = default;
    auto operator=(const OptionErrorContext &) -> OptionErrorContext & = default;
    OptionErrorContext(OptionErrorContext &&) noexcept = default;
    auto operator=(OptionErrorContext &&) noexcept -> OptionErrorContext & = default;

public:
    /// Get the short user-facing error title.
    [[nodiscard]] auto title() const -> text::StringView { return _title; }
    /// Set the short user-facing error title.
    /// @param title The title text.
    /// @return A reference to this context.
    auto setTitle(text::StringView title) -> OptionErrorContext & {
        _title = std::move(title);
        return *this;
    }
    /// Get the detailed user-facing error description.
    [[nodiscard]] auto description() const -> text::StringView { return _description; }
    /// Set the detailed user-facing error description.
    /// @param description The description text.
    /// @return A reference to this context.
    auto setDescription(text::StringView description) -> OptionErrorContext & {
        _description = std::move(description);
        return *this;
    }
    /// Get the machine-readable error reason.
    [[nodiscard]] auto reason() const -> OptionErrorReason { return _reason; }
    /// Set the machine-readable error reason.
    /// @param reason The reason value.
    /// @return A reference to this context.
    auto setReason(OptionErrorReason reason) -> OptionErrorContext & {
        _reason = reason;
        return *this;
    }
    /// Get the command-line argument index related to the error.
    [[nodiscard]] auto argumentIndex() const -> unit::ArgumentIndex { return _argumentIndex; }
    /// Set the command-line argument index related to the error.
    /// @param index The argument index.
    /// @return A reference to this context.
    auto setArgumentIndex(unit::ArgumentIndex index) -> OptionErrorContext & {
        _argumentIndex = index;
        return *this;
    }
    /// Get the options root related to the error.
    [[nodiscard]] auto options() const noexcept -> const OptionsPtr & { return _options; }
    /// Set the options root related to the error.
    /// @param options The options root.
    /// @return A reference to this context.
    auto setOptions(OptionsPtr options) -> OptionErrorContext & {
        _options = std::move(options);
        return *this;
    }
    /// Get the selected module related to the error.
    [[nodiscard]] auto module() const noexcept -> const OptionModulePtr & { return _module; }
    /// Set the selected module related to the error.
    /// @param module The selected module.
    /// @return A reference to this context.
    auto setModule(OptionModulePtr module) -> OptionErrorContext & {
        _module = std::move(module);
        return *this;
    }
    /// Get the option related to the error.
    [[nodiscard]] auto option() const noexcept -> const OptionPtr & { return _option; }
    /// Set the option related to the error.
    /// @param option The option pointer.
    /// @return A reference to this context.
    auto setOption(const OptionPtr &option) -> OptionErrorContext & {
        _option = option;
        return *this;
    }
    /// Get the option set related to the error.
    [[nodiscard]] auto optionSet() const noexcept -> const OptionSetPtr & { return _optionSet; }
    /// Set the option set related to the error.
    /// @param optionSet The option-set pointer.
    /// @return A reference to this context.
    auto setOptionSet(const OptionSetPtr &optionSet) -> OptionErrorContext & {
        _optionSet = optionSet;
        return *this;
    }
    /// Get all command-line arguments related to the error.
    [[nodiscard]] auto arguments() const noexcept -> const text::StringViewList & { return _arguments; }
    /// Set all command-line arguments related to the error.
    /// @param arguments The command-line arguments.
    /// @return A reference to this context.
    auto setArguments(text::StringViewList arguments) -> OptionErrorContext & {
        _arguments = std::move(arguments);
        return *this;
    }
    /// Get the display wording captured for this error.
    [[nodiscard]] auto displayText() const noexcept -> const i18n::DisplayTextMapConstPtr & { return _displayText; }
    /// Set the display wording captured for this error.
    /// @param displayText The wording configuration.
    /// @return A reference to this context.
    auto setDisplayText(i18n::DisplayTextMapConstPtr displayText) -> OptionErrorContext & {
        _displayText = std::move(displayText);
        return *this;
    }

private:
    text::StringView _title;                            ///< The short error title.
    text::StringView _description;                      ///< The detailed error description.
    OptionErrorReason _reason{OptionErrorReason::None}; ///< Machine readable reason for the error.
    unit::ArgumentIndex _argumentIndex{
        unit::ArgumentIndex::noIndex()};                ///< The argument index where the error occurred.
    OptionsPtr _options;                                ///< The options root active for the error.
    OptionModulePtr _module;                            ///< The selected module, if any.
    OptionPtr _option;                                  ///< The option where the error occurred.
    OptionSetPtr _optionSet;                            ///< The option set where the error occurred.
    text::StringViewList _arguments;                    ///< The command-line arguments, if available.
    i18n::DisplayTextMapConstPtr _displayText;          ///< The wording captured for diagnostic rendering.
};

}
