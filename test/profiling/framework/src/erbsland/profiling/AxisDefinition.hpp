// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "AxisDefinition_fwd.hpp"
#include "AxisValue.hpp"

#include <erbsland/all.hpp>

namespace erbsland::profiling {

/// A registered categorical scenario axis.
/// @tested{ProfilingDefinitionTest}
class AxisDefinition final {
public:
    /// Create an empty axis definition.
    AxisDefinition() = default;
    /// Create an axis definition.
    /// @param id Stable internal identifier.
    /// @param configurationName ELCL scenario field name.
    /// @param optionName Command-line filter name without leading dashes.
    AxisDefinition(String id, String configurationName, String optionName);

public:
    /// Add a supported value.
    /// @param value Value to add.
    /// @return This definition.
    auto addValue(AxisValue value) -> AxisDefinition &;
    /// Test whether the axis has the given value.
    [[nodiscard]] auto hasValue(const String &id) const -> bool;

public: // attributes
    /// Access the stable internal identifier.
    [[nodiscard]] auto id() const -> const String & { return _id; }
    /// Access the ELCL configuration field name.
    [[nodiscard]] auto configurationName() const -> const String & { return _configurationName; }
    /// Access the command-line filter name.
    [[nodiscard]] auto optionName() const -> const String & { return _optionName; }
    /// Access the supported axis values.
    [[nodiscard]] auto values() const -> const List<AxisValue> & { return _values; }

private:
    String _id;
    String _configurationName;
    String _optionName;
    List<AxisValue> _values;
};

}
