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
    OptionParserStorage() = default;
    ~OptionParserStorage() = default;

public:
    /// Store a parsed flag.
    [[nodiscard]] auto storeFlag(const OptionPtr &option, unit::ArgumentIndex index) -> bool;
    /// Store a parsed value according to the option type.
    [[nodiscard]] auto storeValue(const OptionPtr &option, const text::StringView &value, unit::ArgumentIndex index)
        -> bool;
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
    [[nodiscard]] static auto optionTitleForError(const OptionPtr &option) -> text::String;
    [[nodiscard]] auto findParsedValue(const OptionPtr &option) -> OptionParsedValuePtr;
    [[nodiscard]] auto storeIntegerValue(const OptionPtr &option, OptionInteger value, unit::ArgumentIndex index)
        -> bool;
    [[nodiscard]] auto storeTextValue(const OptionPtr &option, text::String value, unit::ArgumentIndex index) -> bool;
    [[nodiscard]] auto storeDefaultValue(const OptionPtr &option) -> bool;
    [[nodiscard]] auto storeDefaultIntegerValue(const OptionPtr &option, OptionInteger value) -> bool;
    [[nodiscard]] auto storeDefaultTextValue(const OptionPtr &option, text::String value) -> bool;

    auto makeError(OptionErrorReason reason, text::StringView description, unit::ArgumentIndex index) -> bool;
    auto makeError(
        OptionErrorReason reason, text::StringView description, unit::ArgumentIndex index, const OptionPtr &option)
        -> bool;
    auto makeError(
        OptionErrorReason reason,
        text::StringView title,
        text::StringView description,
        unit::ArgumentIndex index,
        const OptionPtr &option) -> bool;

private:
    std::vector<OptionParsedValuePtr> _parsedValues; ///< Parsed values before final mapping.
    std::optional<OptionErrorContext> _error;        ///< The last storage error.
};

}
