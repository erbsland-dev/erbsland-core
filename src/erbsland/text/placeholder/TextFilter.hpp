// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Filter.hpp"

namespace erbsland::text::placeholder {

/// Built-in text placeholder filters.
/// @tested{ReplacerTest ParserPlaceholderTest}
class TextFilter final : public Filter {
public:
    [[nodiscard]] auto filterNames() const -> StringList override;
    [[nodiscard]] auto apply(const String &filterName, const String &parameter, const String &value) -> String override;
    [[nodiscard]] auto validate(const String &filterName, const String &parameter) -> bool override;

private:
    /// Apply a lower- or upper-case conversion.
    [[nodiscard]] static auto applyCase(const String &filterName, const String &parameter, const String &value)
        -> String;
};

}
