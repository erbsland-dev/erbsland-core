// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Option_fwd.hpp"
#include "OptionValue_fwd.hpp"
#include "OptionValueStorage.hpp"
#include "OptionValueType.hpp"

#include "../unit/ArgumentUnit.hpp"

#include <utility>
#include <vector>

namespace erbsland::options {

/// A single parsed command line option value.
/// @tested{OptionsFrameworkTest}
class OptionValue {
public:
    OptionValue() = default;
    /// Create an option value.
    explicit OptionValue(OptionValueStorage storage);
    /// Create an option value with source argument indexes.
    OptionValue(OptionValueStorage storage, std::vector<unit::ArgumentIndex> argumentIndexes);
    /// Create an option value with its source option.
    OptionValue(OptionWeakPtr option, OptionValueStorage storage);
    /// Create an option value with its source option and argument indexes.
    OptionValue(OptionWeakPtr option, OptionValueStorage storage, std::vector<unit::ArgumentIndex> argumentIndexes);

    // defaults
    ~OptionValue() = default;
    OptionValue(const OptionValue &) = default;
    auto operator=(const OptionValue &) -> OptionValue & = default;
    OptionValue(OptionValue &&) = default;
    auto operator=(OptionValue &&) -> OptionValue & = default;

public:
    /// Create a shared option value.
    [[nodiscard]] static auto create(OptionValueStorage storage) -> OptionValuePtr;
    /// Create a shared option value with source argument indexes.
    [[nodiscard]] static auto create(OptionValueStorage storage, std::vector<unit::ArgumentIndex> argumentIndexes)
        -> OptionValuePtr;
    /// Create a shared option value with its source option.
    [[nodiscard]] static auto create(OptionWeakPtr option, OptionValueStorage storage) -> OptionValuePtr;
    /// Create a shared option value with its source option and argument indexes.
    [[nodiscard]] static auto create(
        OptionWeakPtr option, OptionValueStorage storage, std::vector<unit::ArgumentIndex> argumentIndexes)
        -> OptionValuePtr;

public: // accessors
    /// Get the source option.
    [[nodiscard]] auto option() const noexcept -> const OptionWeakPtr & { return _option; }
    /// Set the source option.
    void setOption(OptionWeakPtr option) noexcept { _option = std::move(option); }
    /// Get the stored value.
    [[nodiscard]] auto storage() const noexcept -> const OptionValueStorage & { return _storage; }
    /// Set the stored value.
    void setStorage(OptionValueStorage storage) { _storage = std::move(storage); }
    /// Get the source argument indexes.
    [[nodiscard]] auto argumentIndexes() const noexcept -> const std::vector<unit::ArgumentIndex> & {
        return _argumentIndexes;
    }
    /// Get the first source argument index.
    [[nodiscard]] auto argumentIndex() const noexcept -> unit::ArgumentIndex;
    /// Set the source argument indexes.
    void setArgumentIndexes(std::vector<unit::ArgumentIndex> argumentIndexes) {
        _argumentIndexes = std::move(argumentIndexes);
    }
    /// Get the number of source occurrences for a flag value.
    [[nodiscard]] auto flagCount() const noexcept -> unit::ArgumentCount;
    /// Get the number of stored values.
    [[nodiscard]] auto valueCount() const noexcept -> unit::ArgumentCount;

public: // typed accessors
    /// Get the concrete value type.
    [[nodiscard]] auto type() const noexcept -> OptionValueType;
    /// Read a flag value.
    [[nodiscard]] auto getFlag(bool defaultFlag = false) const -> bool;
    /// Read an integer value.
    [[nodiscard]] auto getInteger(OptionInteger defaultInteger = 0) const -> OptionInteger;
    /// Read a text value.
    [[nodiscard]] auto getText(const text::StringView &defaultText = {}) const -> text::StringView;
    /// Read a text list value.
    [[nodiscard]] auto getTextList(std::vector<text::StringView> defaultTextList = {}) const
        -> std::vector<text::StringView>;
    /// Read an integer list value.
    [[nodiscard]] auto getIntegerList(std::vector<OptionInteger> defaultIntegerList = {}) const
        -> std::vector<OptionInteger>;

private:
    OptionWeakPtr _option;                               ///< Reference to the option that created this value.
    OptionValueStorage _storage{false};                  ///< The stored option value.
    std::vector<unit::ArgumentIndex> _argumentIndexes{}; ///< The source argument indexes.
};

}
