// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Source.hpp"

#include "../StringMap.hpp"

namespace erbsland::text::placeholder {

/// Built-in application-variable placeholder source.
/// @tested{ReplacerTest ParserPlaceholderTest}
class VariableSource final : public Source {
public:
    /// Create a variable source.
    explicit VariableSource(StringMap<String> variables, const String &name = {});

public:
    [[nodiscard]] auto sourceNames() const -> StringList override;
    [[nodiscard]] auto resolve(const String &sourceName, const String &parameter) -> String override;
    [[nodiscard]] auto validate(const String &sourceName, const String &parameter) -> bool override;

    /// Replace all variables.
    void setVariables(StringMap<String> variables);

private:
    /// Normalize and validate every variable name.
    [[nodiscard]] static auto normalizedVariables(StringMap<String> variables) -> StringMap<String>;

private:
    StringMap<String> _variables;
    String _name; ///< Registered source name.
};

}
