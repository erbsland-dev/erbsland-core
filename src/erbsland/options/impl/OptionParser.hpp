// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

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

    ~OptionParser();

    // deletions
    OptionParser(const OptionParser &) = delete;
    auto operator=(const OptionParser &) -> OptionParser & = delete;
    OptionParser(OptionParser &&) = delete;
    auto operator=(OptionParser &&) -> OptionParser & = delete;

public:
    /// Parse the root options.
    [[nodiscard]] auto parse() -> OptionResult;

    struct NameMatch {
        OptionPtr option;
        bool disabled{false};
    };

    struct PositionalArgument {
        text::String value;
        unit::ArgumentIndex index;
    };

    struct BuiltInFlagMatch {
        OptionResultStatus status;
        bool value{true};
        bool explicitValue{false};
        bool consumedFollowing{false};
        bool validValue{true};
    };

private:
    [[nodiscard]] auto prepareModuleParsing() -> bool;
    [[nodiscard]] auto parseActiveOptions() -> bool;
    [[nodiscard]] auto parseLongOption(const text::String &argument, unit::ArgumentIndex index) -> bool;
    [[nodiscard]] auto parseShortOption(const text::String &argument, unit::ArgumentIndex index) -> bool;
    [[nodiscard]] auto consumeFollowingValue(
        text::String &value, unit::ArgumentIndex optionIndex, const OptionPtr &option) -> bool;
    [[nodiscard]] auto collectPositionalArgument(const text::String &value, unit::ArgumentIndex index) -> bool;
    [[nodiscard]] auto assignPositionals() -> bool;
    [[nodiscard]] auto positionalOptions() const -> std::vector<OptionPtr>;
    [[nodiscard]] auto requiredPositionalsAfter(const std::vector<OptionPtr> &options, std::size_t optionIndex) const
        -> unit::ArgumentCount;
    [[nodiscard]] auto positionalValueLimit(
        const OptionPtr &option,
        const std::vector<OptionPtr> &options,
        std::size_t optionIndex,
        unit::ArgumentCount remainingValues) const -> unit::ArgumentCount;
    [[nodiscard]] auto finishSuccess() -> OptionResult;
    [[nodiscard]] auto finishStatus(OptionResultStatus status) -> OptionResult;
    [[nodiscard]] auto finishError() -> OptionResult;
    [[nodiscard]] auto acceptStorageResult(bool success) -> bool;
    [[nodiscard]] auto storeValue(
        const OptionPtr &option,
        const text::String &value,
        unit::ArgumentIndex argumentIndex,
        unit::ByteIndex startIndex) -> bool;
    void cleanupSensitiveText();
    void cleanupSensitiveTextNoThrow() noexcept;

    [[nodiscard]] auto runSelectedModulePreCallback() -> bool;
    [[nodiscard]] auto runActiveOptionSetPreCallbacks() -> bool;
    [[nodiscard]] auto runValidators() -> bool;
    [[nodiscard]] auto runPostCallbacks() -> bool;
    [[nodiscard]] auto valueForOption(const OptionPtr &option) const -> OptionValuePtr;
    auto makeCallbackError(
        OptionErrorContext context, OptionErrorReason defaultReason, const OptionSetPtr &optionSet = {}) -> bool;
    auto makeValidatorError(OptionErrorContext context, const OptionSetPtr &optionSet, const OptionPtr &option) -> bool;

    [[nodiscard]] auto collectActiveOptionSets() -> std::vector<OptionSetPtr>;
    [[nodiscard]] auto findModule(const text::String &name) const -> OptionModulePtr;
    [[nodiscard]] auto findLongOption(const text::String &name) const -> NameMatch;
    [[nodiscard]] auto findShortOption(text::Char shortName) const -> NameMatch;
    [[nodiscard]] auto isEnabledBuiltInOption(const OptionSetPtr &optionSet, const OptionPtr &option) const -> bool;
    [[nodiscard]] auto isEnabledBuiltInFlag(const text::String &name) const -> bool;
    [[nodiscard]] auto booleanValuesEnabled() const noexcept -> bool;
    [[nodiscard]] static auto parseBooleanLiteral(const text::String &text, bool &value) noexcept -> bool;
    [[nodiscard]] auto builtInFlagAt(unit::ArgumentIndex index) const -> std::optional<BuiltInFlagMatch>;
    [[nodiscard]] auto isHelpOrVersionRequest(OptionResultStatus &status) const -> bool;
    [[nodiscard]] auto isHelpOrVersionRequest(OptionResultStatus &status, unit::ArgumentIndex startIndex) const -> bool;
    [[nodiscard]] auto validateOptionNames() -> bool;

    [[nodiscard]] auto isIndexInArgs(unit::ArgumentIndex index) const -> bool;
    [[nodiscard]] auto getArgAt(unit::ArgumentIndex index) const -> text::String;

    auto makeError(OptionErrorReason reason, text::String description, unit::ArgumentIndex index) -> bool;
    auto makeError(OptionErrorReason reason, text::String title, text::String description, unit::ArgumentIndex index)
        -> bool;
    auto makeError(
        OptionErrorReason reason, text::String description, unit::ArgumentIndex index, const OptionPtr &option) -> bool;
    auto makeError(
        OptionErrorReason reason,
        text::String title,
        text::String description,
        unit::ArgumentIndex index,
        const OptionPtr &option) -> bool;
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
