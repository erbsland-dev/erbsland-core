// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "OptionParser_fwd.hpp"
#include "OptionParserStorage.hpp"

#include "../Option_fwd.hpp"
#include "../OptionErrorContext.hpp"
#include "../OptionErrorReason.hpp"
#include "../OptionModule_fwd.hpp"
#include "../OptionResult_fwd.hpp"
#include "../OptionResultStatus.hpp"
#include "../Options_fwd.hpp"
#include "../OptionSensitiveTextLocation.hpp"
#include "../OptionSet_fwd.hpp"
#include "../OptionValue_fwd.hpp"
#include "../OptionValues_fwd.hpp"

#include "../../core/CommandLineArguments.hpp"
#include "../../i18n/DisplayTextMap_fwd.hpp"
#include "../../unit/ArgumentUnit.hpp"

#include <cstddef>
#include <memory>
#include <optional>
#include <vector>

namespace erbsland::options::impl {

/// Internal option parser.
/// @tested{OptionsParserTest}
class OptionParser final {
public:
    /// Create a parser for the given options and command line arguments.
    OptionParser(OptionsPtr options, core::CommandLineArguments &args, i18n::DisplayTextMapConstPtr displayText);

    /// Destroy the parser after masking sensitive command-line text.
    ~OptionParser();

    // defaults/deletions
    /// Disallow copying parser state that refers to mutable arguments.
    OptionParser(const OptionParser &) = delete;
    /// Disallow assigning parser state that refers to mutable arguments.
    auto operator=(const OptionParser &) -> OptionParser & = delete;
    /// Disallow moving parser state while parsing mutable arguments.
    OptionParser(OptionParser &&) = delete;
    /// Disallow assigning moved parser state that refers to mutable arguments.
    auto operator=(OptionParser &&) -> OptionParser & = delete;

public:
    /// Parse the root options.
    [[nodiscard]] auto parse() -> OptionResult;

    /// Match result for an option name lookup.
    struct NameMatch {
        OptionPtr option;
        bool disabled{false};
    };

    /// Positional command-line argument retained during parsing.
    struct PositionalArgument {
        text::String value;
        unit::ArgumentIndex index;
    };

    /// Match details for a built-in help or version flag.
    struct BuiltInFlagMatch {
        OptionResultStatus status;
        bool value{true};
        bool explicitValue{false};
        bool consumedFollowing{false};
        bool validValue{true};
    };

private:
    /// Select and prepare the requested module for parsing.
    [[nodiscard]] auto prepareModuleParsing() -> bool;
    /// Parse all active command-line options.
    [[nodiscard]] auto parseActiveOptions() -> bool;
    /// Parse a long option argument.
    [[nodiscard]] auto parseLongOption(const text::String &argument, unit::ArgumentIndex index) -> bool;
    /// Parse a short option argument or a group of short options.
    [[nodiscard]] auto parseShortOption(const text::String &argument, unit::ArgumentIndex index) -> bool;
    /// Consume the following argument as an option value when allowed.
    [[nodiscard]] auto consumeFollowingValue(
        text::String &value, unit::ArgumentIndex optionIndex, const OptionPtr &option) -> bool;
    /// Collect one positional argument before assigning it to options.
    [[nodiscard]] auto collectPositionalArgument(const text::String &value, unit::ArgumentIndex index) -> bool;
    /// Assign collected positional arguments to positional options.
    [[nodiscard]] auto assignPositionals() -> bool;
    /// Collect positional options from the active option sets.
    [[nodiscard]] auto positionalOptions() const -> std::vector<OptionPtr>;
    /// Count required positional options following an option index.
    [[nodiscard]] auto requiredPositionalsAfter(const std::vector<OptionPtr> &options, std::size_t optionIndex) const
        -> unit::ArgumentCount;
    /// Determine how many remaining values one positional option may consume.
    [[nodiscard]] auto positionalValueLimit(
        const OptionPtr &option,
        const std::vector<OptionPtr> &options,
        std::size_t optionIndex,
        unit::ArgumentCount remainingValues) const -> unit::ArgumentCount;
    /// Finish parsing with successfully collected values.
    [[nodiscard]] auto finishSuccess() -> OptionResult;
    /// Finish parsing with a requested display status.
    [[nodiscard]] auto finishStatus(OptionResultStatus status) -> OptionResult;
    /// Finish parsing with the recorded error.
    [[nodiscard]] auto finishError() -> OptionResult;
    /// Convert a storage operation result into parser success or failure.
    [[nodiscard]] auto acceptStorageResult(bool success) -> bool;
    /// Store one parsed value and track its sensitive source suffix.
    [[nodiscard]] auto storeValue(
        const OptionPtr &option,
        const text::String &value,
        unit::ArgumentIndex argumentIndex,
        unit::ByteIndex startIndex) -> bool;
    /// Mask every sensitive command-line suffix found during parsing.
    void cleanupSensitiveText();
    /// Mask remaining sensitive text without allowing exceptions to escape.
    void cleanupSensitiveTextNoThrow() noexcept;

    /// Run the selected module callback before parsing.
    [[nodiscard]] auto runSelectedModulePreCallback() -> bool;
    /// Run active option-set callbacks before parsing.
    [[nodiscard]] auto runActiveOptionSetPreCallbacks() -> bool;
    /// Validate every parsed option value.
    [[nodiscard]] auto runValidators() -> bool;
    /// Run callbacks after successful parsing.
    [[nodiscard]] auto runPostCallbacks() -> bool;
    /// Find the parsed value associated with an option.
    [[nodiscard]] auto valueForOption(const OptionPtr &option) const -> OptionValuePtr;
    /// Convert a callback exception into a parser error.
    auto makeCallbackError(
        OptionErrorContext context, OptionErrorReason defaultReason, const OptionSetPtr &optionSet = {}) -> bool;
    /// Convert a validator exception into a parser error.
    auto makeValidatorError(OptionErrorContext context, const OptionSetPtr &optionSet, const OptionPtr &option) -> bool;

    /// Collect option sets active for the selected module.
    [[nodiscard]] auto collectActiveOptionSets() -> std::vector<OptionSetPtr>;
    /// Find a module by its command-line name.
    [[nodiscard]] auto findModule(const text::String &name) const -> OptionModulePtr;
    /// Find a long option by name among active sets.
    [[nodiscard]] auto findLongOption(const text::String &name) const -> NameMatch;
    /// Find a short option by its single-character name.
    [[nodiscard]] auto findShortOption(text::Char shortName) const -> NameMatch;
    /// Test whether an option is an enabled built-in option.
    [[nodiscard]] auto isEnabledBuiltInOption(const OptionSetPtr &optionSet, const OptionPtr &option) const -> bool;
    /// Test whether a name identifies an enabled built-in flag.
    [[nodiscard]] auto isEnabledBuiltInFlag(const text::String &name) const -> bool;
    /// Test whether explicit boolean flag values are enabled.
    [[nodiscard]] auto booleanValuesEnabled() const noexcept -> bool;
    /// Parse a recognized boolean literal.
    [[nodiscard]] static auto parseBooleanLiteral(const text::String &text, bool &value) noexcept -> bool;
    /// Get the built-in flag match at an argument index.
    [[nodiscard]] auto builtInFlagAt(unit::ArgumentIndex index) const -> std::optional<BuiltInFlagMatch>;
    /// Test whether any argument requests help or version output.
    [[nodiscard]] auto isHelpOrVersionRequest(OptionResultStatus &status) const -> bool;
    /// Test whether an argument suffix requests help or version output.
    [[nodiscard]] auto isHelpOrVersionRequest(OptionResultStatus &status, unit::ArgumentIndex startIndex) const -> bool;
    /// Validate option names in every active option set.
    [[nodiscard]] auto validateOptionNames() -> bool;

    /// Test whether an argument index is valid for the input arguments.
    [[nodiscard]] auto isIndexInArgs(unit::ArgumentIndex index) const -> bool;
    /// Get the command-line argument at a valid index.
    [[nodiscard]] auto getArgAt(unit::ArgumentIndex index) const -> text::String;

    /// Record a parser error with a default title.
    auto makeError(OptionErrorReason reason, text::String description, unit::ArgumentIndex index) -> bool;
    /// Record a parser error with an explicit title.
    auto makeError(OptionErrorReason reason, text::String title, text::String description, unit::ArgumentIndex index)
        -> bool;
    /// Record a parser error associated with an option.
    auto makeError(
        OptionErrorReason reason, text::String description, unit::ArgumentIndex index, const OptionPtr &option) -> bool;
    /// Record a titled parser error associated with an option.
    auto makeError(
        OptionErrorReason reason,
        text::String title,
        text::String description,
        unit::ArgumentIndex index,
        const OptionPtr &option) -> bool;
    /// Record a complete parser error context.
    auto makeError(OptionErrorContext context) -> bool;

private:
    OptionsPtr _options;                                  ///< The options root.
    core::CommandLineArguments &_args;                    ///< The mutable arguments to parse and mask.
    i18n::DisplayTextMapConstPtr _displayText;            ///< The wording captured for diagnostics.
    OptionModulePtr _selectedModule;                      ///< The selected module, if any.
    text::String _moduleName;                             ///< The canonical selected module name.
    std::vector<OptionSetPtr> _activeOptionSets;          ///< The option sets active for parsing.
    OptionParserStorage _storage;                         ///< The parsed option values.
    OptionValuesPtr _values;                              ///< The final parsed values.
    std::vector<PositionalArgument> _positionals;         ///< Positional values found while parsing.
    unit::ArgumentIndex _argumentIndex;                   ///< Current parser position.
    unit::ArgumentIndex _moduleArgumentIndex;             ///< The selected module argument index.
    std::optional<OptionErrorContext> _error;             ///< The first parse error.
    OptionSensitiveTextLocations _sensitiveTextLocations; ///< Sensitive argument suffixes found during parsing.
    std::size_t _sensitiveTextCleanupIndex{0};            ///< The next sensitive location to clean.
};

}
