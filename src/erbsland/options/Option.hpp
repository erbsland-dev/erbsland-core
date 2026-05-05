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

#include "../text/String.hpp"
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
    Option() = default;
    /// Create an option with names.
    Option(std::initializer_list<text::StringView> names);

    // defaults
    ~Option() = default;
    Option(const Option &) = default;
    auto operator=(const Option &) -> Option & = default;
    Option(Option &&) = default;
    auto operator=(Option &&) -> Option & = default;

public:
    /// Create a shared option.
    [[nodiscard]] static auto create() -> OptionPtr;
    /// Create a shared option with names.
    [[nodiscard]] static auto create(std::initializer_list<text::StringView> names) -> OptionPtr;
    /// Test if a name is a long command line option name.
    [[nodiscard]] static auto isLongName(const text::StringView &name) noexcept -> bool;
    /// Test if a name is a short command line option name.
    [[nodiscard]] static auto isShortName(const text::StringView &name) noexcept -> bool;
    /// Test if a name is a command line option name.
    [[nodiscard]] static auto isOptionName(const text::StringView &name) noexcept -> bool;
    /// Test if a name is a positional argument name.
    [[nodiscard]] static auto isPositionalName(const text::StringView &name) noexcept -> bool;
    /// Test if a name is a valid long command line option name.
    [[nodiscard]] static auto isValidLongName(const text::StringView &name) noexcept -> bool;
    /// Test if a name is a valid short command line option name.
    [[nodiscard]] static auto isValidShortName(const text::StringView &name) noexcept -> bool;
    /// Test if a name is a valid command line option name.
    [[nodiscard]] static auto isValidOptionName(const text::StringView &name) noexcept -> bool;
    /// Test if a name is a valid positional argument name.
    [[nodiscard]] static auto isValidPositionalName(const text::StringView &name) noexcept -> bool;
    /// Add a name for this option.
    void addName(text::StringView name);

public: // accessors
    /// Get all names for this option.
    [[nodiscard]] auto names() const noexcept -> const std::vector<text::StringView> & { return _names; }
    /// Replace all names for this option.
    void setNames(std::initializer_list<text::StringView> names);
    /// Test if this option is disabled.
    [[nodiscard]] auto isDisabled() const noexcept -> bool;
    /// Test if this option has the given long name.
    [[nodiscard]] auto hasLongName(const text::StringView &name) const -> bool;
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
    [[nodiscard]] auto hasPositionalName(const text::StringView &name) const -> bool;
    /// Test if all names in this option are valid.
    [[nodiscard]] auto hasValidOptionNames() const noexcept -> bool;
    /// Test if this option has a name that conflicts with another option.
    [[nodiscard]] auto hasConflictingOptionName(const Option &other) const -> bool;
    /// Find a configured choice text matching `text` case-insensitively.
    [[nodiscard]] auto matchingChoiceText(const text::StringView &text) const -> std::optional<text::StringView>;
    /// Get the help text.
    [[nodiscard]] auto help() const noexcept -> const OptionHelp & { return _help; }
    /// Set the help text.
    void setHelp(OptionHelp help) { _help = std::move(help); }
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
    void setValidateFn(OptionValidateFn fn) { _validateFn = std::move(fn); }

private:
    void updateImplicitTypeFromNames() noexcept;

private:
    std::vector<text::StringView> _names;                     ///< The names for this option.
    OptionHelp _help;                                         ///< The help text for the option.
    OptionType _type;                                         ///< The value type.
    bool _explicitType{false};                                ///< Whether the type was explicitly set.
    OptionFlags _flags;                                       ///< The option flags.
    OptionChoicesPtr _choices;                                ///< The accepted choices.
    unit::ArgumentCount _maximum{unit::ArgumentCount::one()}; ///< The maximum number of values.
    std::optional<OptionValueStorage> _defaultValue;          ///< The default value if not defined.
    OptionValidateFn _validateFn;                             ///< A callback to validate the value of this option.
};

}
