// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Option_fwd.hpp"
#include "OptionCallback.hpp"
#include "OptionChoices_fwd.hpp"
#include "OptionFlag.hpp"
#include "OptionHelp.hpp"
#include "OptionType.hpp"
#include "OptionValueStorage.hpp"

#include "../text/StringEditor.hpp"
#include "../unit/ArgumentUnit.hpp"

#include <initializer_list>
#include <optional>
#include <utility>
#include <vector>

namespace erbsland::options {

/// A single command line option definition.
/// An option with at least one dashed name is a regular option. Dashless names on regular options are value aliases.
/// An option without dashed names is a positional argument.
/// @seedoc{/reference/options/command_line_tools}
/// @tested{OptionsFrameworkTest}
class Option {
public:
    /// Create an empty option.
    Option() = default;
    /// Create an option with names.
    Option(std::initializer_list<text::String> names);

    // defaults
    ~Option() = default;
    Option(const Option &) = default;
    auto operator=(const Option &) -> Option & = default;
    Option(Option &&) = default;
    auto operator=(Option &&) -> Option & = default;

public:
    /// Create a shared empty option.
    /// @return A shared option with no names and the default implicit text type.
    [[nodiscard]] static auto create() -> OptionPtr;
    /// Create a shared option with names.
    /// @param names The command-line names and lookup aliases for this option.
    /// @return A shared option whose implicit type is derived from the names.
    [[nodiscard]] static auto create(std::initializer_list<text::String> names) -> OptionPtr;
    /// Test if a name is a long command line option name.
    /// @param name The name to classify.
    /// @return `true` if the name starts with `--`.
    [[nodiscard]] static auto isLongName(const text::String &name) noexcept -> bool;
    /// Test if a name is a short command line option name.
    /// @param name The name to classify.
    /// @return `true` if the name starts with one dash and contains exactly one option character.
    [[nodiscard]] static auto isShortName(const text::String &name) noexcept -> bool;
    /// Test if a name is a command line option name.
    [[nodiscard]] static auto isOptionName(const text::String &name) noexcept -> bool;
    /// Test if a name is a positional argument name.
    [[nodiscard]] static auto isPositionalName(const text::String &name) noexcept -> bool;
    /// Test if a name is a valid long command line option name.
    [[nodiscard]] static auto isValidLongName(const text::String &name) noexcept -> bool;
    /// Test if a name is a valid short command line option name.
    [[nodiscard]] static auto isValidShortName(const text::String &name) noexcept -> bool;
    /// Test if a name is a valid command line option name.
    [[nodiscard]] static auto isValidOptionName(const text::String &name) noexcept -> bool;
    /// Test if a name is a valid positional argument name.
    [[nodiscard]] static auto isValidPositionalName(const text::String &name) noexcept -> bool;
    /// Add a command-line name or lookup alias for this option.
    /// @param name The name to append. Dashed names are accepted on the command line, dashless names are lookup aliases
    ///     or positional argument names.
    void addName(text::String name);

public: // accessors
    /// Get all names for this option.
    [[nodiscard]] auto names() const noexcept -> const std::vector<text::String> & { return _names; }
    /// Replace all names for this option.
    void setNames(std::initializer_list<text::String> names);
    /// Test if this option is disabled.
    [[nodiscard]] auto isDisabled() const noexcept -> bool;
    /// Test if this option has the given long name.
    [[nodiscard]] auto hasLongName(const text::String &name) const -> bool;
    /// Test if this option has the given short name.
    [[nodiscard]] auto hasShortName(text::Char shortName) const -> bool;
    /// Test if this option has any dashed regular option name.
    [[nodiscard]] auto hasOptionName() const -> bool;
    /// Test if this option is a regular option.
    [[nodiscard]] auto isRegularOption() const -> bool { return hasOptionName(); }
    /// Test if this option is a positional argument.
    [[nodiscard]] auto isPositionalArgument() const -> bool { return !hasOptionName(); }
    /// Test if this option has any dashless name.
    [[nodiscard]] auto hasPositionalName() const -> bool;
    /// Test if this option has the given dashless name.
    [[nodiscard]] auto hasPositionalName(const text::String &name) const -> bool;
    /// Test if all names in this option are valid.
    [[nodiscard]] auto hasValidOptionNames() const noexcept -> bool;
    /// Test if this option has a name that conflicts with another option.
    [[nodiscard]] auto hasConflictingOptionName(const Option &other) const -> bool;
    /// Find a configured choice text matching `text` case-insensitively.
    [[nodiscard]] auto matchingChoiceText(const text::String &text) const -> std::optional<text::String>;
    /// Get the help metadata for this option.
    [[nodiscard]] auto help() const noexcept -> const OptionHelp & { return _help; }
    /// Set the complete help metadata for this option.
    /// @param help The replacement help metadata.
    void setHelp(OptionHelp help) { _help = std::move(help); }
    /// Set the help title for this option.
    /// @param title Short title used when no description is available.
    void setHelpTitle(text::String title) { _help.setTitle(std::move(title)); }
    /// Set the help description for this option.
    /// @param description User-facing description shown next to this option in help output.
    void setHelpDescription(text::String description) { _help.setDescription(std::move(description)); }
    /// Set the help epilog for this option.
    /// @param epilog Optional trailing text for renderers that support option-level epilogs.
    void setHelpEpilog(text::String epilog) { _help.setEpilog(std::move(epilog)); }
    /// Set the help visibility for this option.
    /// @param visibility Controls where this option appears in generated help output.
    void setHelpVisibility(const OptionHelpVisibility visibility) noexcept { _help.setVisibility(visibility); }
    /// Get the custom value name shown in help output.
    [[nodiscard]] auto valueName() const noexcept -> const text::String & { return _valueName; }
    /// Set the custom value name shown in help output.
    /// @param valueName Bare value name without angle brackets. Empty restores the type-derived default.
    void setValueName(text::String valueName) { _valueName = std::move(valueName); }
    /// Get the option type.
    [[nodiscard]] auto type() const noexcept -> OptionType { return _type; }
    /// Set the option type.
    void setType(const OptionType type) noexcept {
        _type = type;
        _explicitType = true;
    }
    /// Get the option flags.
    [[nodiscard]] auto flags() const noexcept -> OptionFlags { return _flags; }
    /// Set the option flags.
    void setFlags(const OptionFlags flags) noexcept { _flags = flags; }
    /// Get the accepted choices.
    [[nodiscard]] auto choices() const noexcept -> const OptionChoicesPtr & { return _choices; }
    /// Set the accepted choices.
    void setChoices(OptionChoicesPtr choices) { _choices = std::move(choices); }
    /// Get the maximum number of values for this option.
    [[nodiscard]] auto maximum() const noexcept -> unit::ArgumentCount { return _maximum; }
    /// Set the maximum number of values for this option.
    void setMaximum(const unit::ArgumentCount maximum) noexcept { _maximum = maximum; }
    /// Test if a default value is set.
    [[nodiscard]] auto hasDefaultValue() const noexcept -> bool { return _defaultValue.has_value(); }
    /// Get the optional default value.
    [[nodiscard]] auto defaultValue() const noexcept -> const std::optional<OptionValueStorage> & {
        return _defaultValue;
    }
    /// Set the optional default value.
    void setDefaultValue(std::optional<OptionValueStorage> defaultValue) { _defaultValue = std::move(defaultValue); }
    /// Get the value validation callback.
    [[nodiscard]] auto validateFn() const noexcept -> const OptionValidateFn & { return _validateFn; }
    /// Set the value validation callback.
    /// @param fn The new value validation callback `(OptionValuePtr valueToValidate, OptionValuesPtr values) -> void`
    void setValidateFn(OptionValidateFn fn) { _validateFn = std::move(fn); }

private:
    /// Update the implicit value type after changing the option names.
    void updateImplicitTypeFromNames() noexcept;

private:
    std::vector<text::String> _names;                         ///< The names for this option.
    OptionHelp _help;                                         ///< The help text for the option.
    text::String _valueName;                                  ///< The custom value name for help output.
    OptionType _type;                                         ///< The value type.
    bool _explicitType{false};                                ///< Whether the type was explicitly set.
    OptionFlags _flags;                                       ///< The option flags.
    OptionChoicesPtr _choices;                                ///< The accepted choices.
    unit::ArgumentCount _maximum{unit::ArgumentCount::one()}; ///< The maximum number of values.
    std::optional<OptionValueStorage> _defaultValue;          ///< The default value if not defined.
    OptionValidateFn _validateFn;                             ///< A callback to validate the value of this option.
};

}
