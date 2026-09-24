// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../text/StringMap.hpp"
#include "../../PlaceholderSource.hpp"

namespace erbsland::conf::impl::placeholder {

/// Built-in application-variable placeholder source.
/// @tested{ParserPlaceholderTest}
class VariablePlaceholderSource final : public PlaceholderSource {
public:
    /// Create a variable source.
    explicit VariablePlaceholderSource(text::StringMap<text::String> variables);

public:
    [[nodiscard]] auto sourceNames() const -> text::StringList override;
    [[nodiscard]] auto resolve(const text::String &sourceName, const text::String &parameter) -> text::String override;

    /// Replace all variables.
    void setVariables(text::StringMap<text::String> variables);

private:
    /// Normalize and validate every variable name.
    [[nodiscard]] static auto normalizedVariables(text::StringMap<text::String> variables)
        -> text::StringMap<text::String>;

private:
    text::StringMap<text::String> _variables;
};

}
