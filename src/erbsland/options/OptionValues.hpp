// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "OptionModule_fwd.hpp"
#include "OptionValue.hpp"
#include "OptionValueNameHash.hpp"
#include "OptionValues_fwd.hpp"

#include "../text/String.hpp"
#include "../unit/ArgumentUnit.hpp"

#include <unordered_map>
#include <vector>

namespace erbsland::options {

/// A set of parsed command line option values.
///
/// Each accepted lookup name maps to the same parsed value instance. For example, an option registered as
/// `{"--verbose", "verbose"}` can be read through either name.
/// @tested{OptionsFrameworkTest}
class OptionValues {
public:
    OptionValues() = default;

    // defaults
    ~OptionValues() = default;
    OptionValues(const OptionValues &) = default;
    auto operator=(const OptionValues &) -> OptionValues & = default;
    OptionValues(OptionValues &&) = default;
    auto operator=(OptionValues &&) -> OptionValues & = default;

public:
    /// Create an empty shared value set.
    /// @return A shared value set without selected module or parsed values.
    [[nodiscard]] static auto create() -> OptionValuesPtr;
    /// Set a value for one lookup name.
    /// @param name Lookup name to associate with the value.
    /// @param value Parsed value pointer. Null pointers are stored as-is and behave like absent values in typed
    /// getters.
    void setValue(const text::StringView &name, OptionValuePtr value);
    /// Set a value for multiple lookup names.
    /// @param names Lookup names that shall all resolve to the same value.
    /// @param value Parsed value pointer.
    void setValue(std::initializer_list<text::StringView> names, OptionValuePtr value);

public: // accessors
    /// Get the selected module name.
    [[nodiscard]] auto moduleName() const noexcept -> const text::StringView & { return _moduleName; }
    /// Set the selected module name.
    /// @param moduleName The module name selected on the command line, or empty for root parsing.
    void setModuleName(text::StringView moduleName) { _moduleName = std::move(moduleName); }
    /// Get the selected module.
    [[nodiscard]] auto module() const noexcept -> const OptionModulePtr & { return _module; }
    /// Set the selected module.
    /// @param module The selected module object, or null for root parsing.
    void setModule(OptionModulePtr module) noexcept { _module = std::move(module); }
    /// Get the number of values stored for a lookup name.
    /// @param name Lookup name to inspect.
    /// @return The number of values or flag occurrences stored for the lookup name.
    [[nodiscard]] auto valueCount(const text::StringView &name) const -> unit::ArgumentCount;
    /// Get the parsed value for a lookup name.
    /// @param name Lookup name to inspect.
    /// @return The parsed value, or null if the name was not present and no default was stored.
    [[nodiscard]] auto value(const text::StringView &name) const -> OptionValuePtr;
    /// Get all mapped values.
    [[nodiscard]] auto values() const noexcept
        -> const std::unordered_map<text::String, OptionValuePtr, OptionValueNameHash> & {
        return _values;
    }

public: // typed accessors
    /// Read a flag value.
    /// @param name Lookup name to read.
    /// @param defaultFlag Returned when the name is absent or not a flag.
    /// @return `true` if the flag was present at least once.
    [[nodiscard]] auto getFlag(const text::StringView &name, bool defaultFlag = false) const -> bool;
    /// Read the number of source occurrences for a flag value.
    /// @param name Lookup name to read.
    /// @param defaultCount Returned when the name is absent or not a flag.
    /// @return The number of flag occurrences.
    [[nodiscard]] auto getFlagCount(const text::StringView &name, unit::ArgumentCount defaultCount = {}) const
        -> unit::ArgumentCount;
    /// Read an integer value.
    /// @param name Lookup name to read.
    /// @param defaultInteger Returned when the name is absent or not an integer.
    /// @return The stored integer value.
    [[nodiscard]] auto getInteger(const text::StringView &name, OptionInteger defaultInteger = 0) const
        -> OptionInteger;
    /// Read a text value.
    /// @param name Lookup name to read.
    /// @param defaultText Returned when the name is absent or not text/choice storage.
    /// @return The stored text or choice value.
    [[nodiscard]] auto getText(const text::StringView &name, const text::StringView &defaultText = {}) const
        -> text::StringView;
    /// Read a text list value.
    /// @param name Lookup name to read.
    /// @param defaultTextList Returned when the name is absent or not a text list.
    /// @return The stored text values.
    [[nodiscard]] auto getTextList(
        const text::StringView &name, std::vector<text::StringView> defaultTextList = {}) const
        -> std::vector<text::StringView>;
    /// Read an integer list value.
    /// @param name Lookup name to read.
    /// @param defaultIntegerList Returned when the name is absent or not an integer list.
    /// @return The stored integer values.
    [[nodiscard]] auto getIntegerList(
        const text::StringView &name, std::vector<OptionInteger> defaultIntegerList = {}) const
        -> std::vector<OptionInteger>;

private:
    text::StringView _moduleName;                                                  ///< The selected module name.
    OptionModulePtr _module;                                                       ///< The selected module.
    std::unordered_map<text::String, OptionValuePtr, OptionValueNameHash> _values; ///< The mapped values.
};

}
