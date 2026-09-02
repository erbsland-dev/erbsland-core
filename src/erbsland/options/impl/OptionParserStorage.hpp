// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "OptionParsedValue.hpp"

#include "../Option_fwd.hpp"
#include "../OptionErrorContext.hpp"
#include "../OptionErrorReason.hpp"
#include "../OptionSet_fwd.hpp"
#include "../OptionValues_fwd.hpp"
#include "../OptionValueStorage.hpp"

#include "../../unit/ArgumentUnit.hpp"

#include <optional>
#include <vector>

namespace erbsland::options::impl {

/// Stores parsed option values and applies option value rules.
/// @tested{OptionsParserTest}
class OptionParserStorage final {
public:
    // defaults
    OptionParserStorage() = default;
    ~OptionParserStorage() = default;

public:
    /// Store a parsed flag.
    /// @param option The flag definition.
    /// @param index The source option argument.
    [[nodiscard]] auto storeFlag(const OptionPtr &option, unit::ArgumentIndex index) -> bool;
    /// Store a parsed boolean value.
    [[nodiscard]] auto storeBooleanValue(const OptionPtr &option, bool value, unit::ArgumentIndex index) -> bool;
    /// Store a parsed value according to the option type.
    [[nodiscard]] auto storeValue(const OptionPtr &option, text::String value, unit::ArgumentIndex index) -> bool;
    /// Apply defaults for all absent options.
    [[nodiscard]] auto applyDefaults(const std::vector<OptionSetPtr> &optionSets) -> bool;
    /// Check required options.
    [[nodiscard]] auto checkRequiredOptions(const std::vector<OptionSetPtr> &optionSets) -> bool;
    /// Test if a value has already been stored for an option.
    [[nodiscard]] auto hasValue(const OptionPtr &option) const -> bool;
    /// Create the public option values map.
    [[nodiscard]] auto values() const -> OptionValuesPtr;
    /// Get the last error context.
    [[nodiscard]] auto errorContext() const noexcept -> const std::optional<OptionErrorContext> & { return _error; }

private:
    /// Create a display title for an option-related error.
    [[nodiscard]] static auto optionTitleForError(const OptionPtr &option) -> text::String;
    /// Find mutable parsed storage for an option.
    [[nodiscard]] auto findParsedValue(const OptionPtr &option) -> OptionParsedValuePtr;
    /// Store a parsed integer option value.
    [[nodiscard]] auto storeIntegerValue(const OptionPtr &option, OptionInteger value, unit::ArgumentIndex index)
        -> bool;
    /// Store a parsed text option value.
    [[nodiscard]] auto storeTextValue(const OptionPtr &option, text::String value, unit::ArgumentIndex index) -> bool;
    /// Store a parsed sensitive text option value.
    [[nodiscard]] auto storeSensitiveTextValue(const OptionPtr &option, text::String value, unit::ArgumentIndex index)
        -> bool;
    /// Store the configured default value for an option.
    [[nodiscard]] auto storeDefaultValue(const OptionPtr &option) -> bool;
    /// Store the configured default integer value for an option.
    [[nodiscard]] auto storeDefaultIntegerValue(const OptionPtr &option, OptionInteger value) -> bool;
    /// Store the configured default text value for an option.
    [[nodiscard]] auto storeDefaultTextValue(const OptionPtr &option, text::String value) -> bool;

    /// Record a storage error with a default title.
    auto makeError(OptionErrorReason reason, text::String description, unit::ArgumentIndex index) -> bool;
    /// Record a storage error associated with an option.
    auto makeError(
        OptionErrorReason reason, text::String description, unit::ArgumentIndex index, const OptionPtr &option) -> bool;
    /// Record a storage error with an explicit title and associated option.
    auto makeError(
        OptionErrorReason reason,
        text::String title,
        text::String description,
        unit::ArgumentIndex index,
        const OptionPtr &option) -> bool;

private:
    std::vector<OptionParsedValuePtr> _parsedValues; ///< Parsed values before final mapping.
    std::optional<OptionErrorContext> _error;        ///< The last storage error.
};

}
