// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Option_fwd.hpp"
#include "OptionValue_fwd.hpp"
#include "OptionValueStorage.hpp"
#include "OptionValueType.hpp"

#include "../text/String.hpp"
#include "../unit/ArgumentUnit.hpp"

#include <utility>
#include <vector>

namespace erbsland::options {

/// A single parsed command line option value.
///
/// Values keep the parsed storage, an optional weak reference to the source option, and the original argument indexes
/// that produced the value.
/// @tested{OptionsFrameworkTest}
class OptionValue {
public:
    OptionValue() = default;
    /// Create an option value.
    /// @param storage Parsed value storage.
    explicit OptionValue(OptionValueStorage storage);
    /// Create an option value with source argument indexes.
    /// @param storage Parsed value storage.
    /// @param argumentIndexes Source argument indexes that produced this value.
    OptionValue(OptionValueStorage storage, std::vector<unit::ArgumentIndex> argumentIndexes);
    /// Create an option value with its source option.
    /// @param option Source option definition.
    /// @param storage Parsed value storage.
    OptionValue(OptionWeakPtr option, OptionValueStorage storage);
    /// Create an option value with its source option and argument indexes.
    /// @param option Source option definition.
    /// @param storage Parsed value storage.
    /// @param argumentIndexes Source argument indexes that produced this value.
    OptionValue(OptionWeakPtr option, OptionValueStorage storage, std::vector<unit::ArgumentIndex> argumentIndexes);

    // defaults
    ~OptionValue() = default;
    OptionValue(const OptionValue &) = default;
    auto operator=(const OptionValue &) -> OptionValue & = default;
    OptionValue(OptionValue &&) = default;
    auto operator=(OptionValue &&) -> OptionValue & = default;

public:
    /// Create a shared option value.
    /// @param storage Parsed value storage.
    /// @return A shared parsed value.
    [[nodiscard]] static auto create(OptionValueStorage storage) -> OptionValuePtr;
    /// Create a shared option value with source argument indexes.
    /// @param storage Parsed value storage.
    /// @param argumentIndexes Source argument indexes that produced this value.
    /// @return A shared parsed value.
    [[nodiscard]] static auto create(OptionValueStorage storage, std::vector<unit::ArgumentIndex> argumentIndexes)
        -> OptionValuePtr;
    /// Create a shared option value with its source option.
    /// @param option Source option definition.
    /// @param storage Parsed value storage.
    /// @return A shared parsed value.
    [[nodiscard]] static auto create(OptionWeakPtr option, OptionValueStorage storage) -> OptionValuePtr;
    /// Create a shared option value with its source option and argument indexes.
    /// @param option Source option definition.
    /// @param storage Parsed value storage.
    /// @param argumentIndexes Source argument indexes that produced this value.
    /// @return A shared parsed value.
    [[nodiscard]] static auto create(
        OptionWeakPtr option, OptionValueStorage storage, std::vector<unit::ArgumentIndex> argumentIndexes)
        -> OptionValuePtr;

public: // accessors
    /// Get the source option.
    [[nodiscard]] auto option() const noexcept -> const OptionWeakPtr & { return _option; }
    /// Set the source option.
    /// @param option Source option definition.
    void setOption(OptionWeakPtr option) noexcept { _option = std::move(option); }
    /// Get the stored value.
    [[nodiscard]] auto storage() const noexcept -> const OptionValueStorage & { return _storage; }
    /// Set the stored value.
    /// @param storage Replacement parsed value storage.
    void setStorage(OptionValueStorage storage) { _storage = std::move(storage); }
    /// Get the source argument indexes.
    [[nodiscard]] auto argumentIndexes() const noexcept -> const std::vector<unit::ArgumentIndex> & {
        return _argumentIndexes;
    }
    /// Get the first source argument index.
    /// @return The first source argument index, or an empty index if no index is stored.
    [[nodiscard]] auto argumentIndex() const noexcept -> unit::ArgumentIndex;
    /// Set the source argument indexes.
    /// @param argumentIndexes Replacement source argument indexes.
    void setArgumentIndexes(std::vector<unit::ArgumentIndex> argumentIndexes) {
        _argumentIndexes = std::move(argumentIndexes);
    }
    /// Get the number of source occurrences for a flag value.
    /// @return The number of stored argument indexes for flag storage.
    [[nodiscard]] auto flagCount() const noexcept -> unit::ArgumentCount;
    /// Get the number of stored values.
    /// @return One for scalar storage, the list length for list storage, or the flag occurrence count for flags.
    [[nodiscard]] auto valueCount() const noexcept -> unit::ArgumentCount;

public: // typed accessors
    /// Get the concrete value type.
    /// @return The concrete storage type.
    [[nodiscard]] auto type() const noexcept -> OptionValueType;
    /// Read a flag value.
    /// @param defaultFlag Returned when this value is not flag storage.
    /// @return The flag value.
    [[nodiscard]] auto getFlag(bool defaultFlag = false) const -> bool;
    /// Read an integer value.
    /// @param defaultInteger Returned when this value is not integer storage.
    /// @return The integer value.
    [[nodiscard]] auto getInteger(OptionInteger defaultInteger = 0) const -> OptionInteger;
    /// Read a text value.
    /// @param defaultText Returned when this value is not text storage.
    /// @return The text or choice value.
    [[nodiscard]] auto getText(const text::String &defaultText = {}) const -> text::String;
    /// Read a text list value.
    /// @param defaultTextList Returned when this value is not text-list storage.
    /// @return The text values.
    [[nodiscard]] auto getTextList(std::vector<text::String> defaultTextList = {}) const -> std::vector<text::String>;
    /// Read an integer list value.
    /// @param defaultIntegerList Returned when this value is not integer-list storage.
    /// @return The integer values.
    [[nodiscard]] auto getIntegerList(std::vector<OptionInteger> defaultIntegerList = {}) const
        -> std::vector<OptionInteger>;

private:
    OptionWeakPtr _option;                               ///< Reference to the option that created this value.
    OptionValueStorage _storage{false};                  ///< The stored option value.
    std::vector<unit::ArgumentIndex> _argumentIndexes{}; ///< The source argument indexes.
};

}
