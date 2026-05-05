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
/// Each accepted lookup name maps to the same parsed value instance.
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
    [[nodiscard]] static auto create() -> OptionValuesPtr;
    /// Set a value for one lookup name.
    void setValue(const text::StringView &name, OptionValuePtr value);
    /// Set a value for multiple lookup names.
    void setValue(std::initializer_list<text::StringView> names, OptionValuePtr value);

public: // accessors
    /// Get the selected module name.
    [[nodiscard]] auto moduleName() const noexcept -> const text::StringView & { return _moduleName; }
    /// Set the selected module name.
    void setModuleName(text::StringView moduleName) { _moduleName = std::move(moduleName); }
    /// Get the selected module.
    [[nodiscard]] auto module() const noexcept -> const OptionModulePtr & { return _module; }
    /// Set the selected module.
    void setModule(OptionModulePtr module) noexcept { _module = std::move(module); }
    /// Get the number of values stored for a lookup name.
    [[nodiscard]] auto valueCount(const text::StringView &name) const -> unit::ArgumentCount;
    /// Get the parsed value for a lookup name.
    [[nodiscard]] auto value(const text::StringView &name) const -> OptionValuePtr;
    /// Get all mapped values.
    [[nodiscard]] auto values() const noexcept
        -> const std::unordered_map<text::String, OptionValuePtr, OptionValueNameHash> & {
        return _values;
    }

public: // typed accessors
    /// Read a flag value.
    [[nodiscard]] auto getFlag(const text::StringView &name, bool defaultFlag = false) const -> bool;
    /// Read the number of source occurrences for a flag value.
    [[nodiscard]] auto getFlagCount(const text::StringView &name, unit::ArgumentCount defaultCount = {}) const
        -> unit::ArgumentCount;
    /// Read an integer value.
    [[nodiscard]] auto getInteger(const text::StringView &name, OptionInteger defaultInteger = 0) const
        -> OptionInteger;
    /// Read a text value.
    [[nodiscard]] auto getText(const text::StringView &name, const text::StringView &defaultText = {}) const
        -> text::StringView;
    /// Read a text list value.
    [[nodiscard]] auto getTextList(
        const text::StringView &name, std::vector<text::StringView> defaultTextList = {}) const
        -> std::vector<text::StringView>;
    /// Read an integer list value.
    [[nodiscard]] auto getIntegerList(
        const text::StringView &name, std::vector<OptionInteger> defaultIntegerList = {}) const
        -> std::vector<OptionInteger>;

private:
    text::StringView _moduleName;                                                  ///< The selected module name.
    OptionModulePtr _module;                                                       ///< The selected module.
    std::unordered_map<text::String, OptionValuePtr, OptionValueNameHash> _values; ///< The mapped values.
};

}
