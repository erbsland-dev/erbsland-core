// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../PlaceholderFilter.hpp"

namespace erbsland::conf::impl::placeholder {

/// Built-in text placeholder filters.
/// @tested{ParserPlaceholderTest}
class TextPlaceholderFilter final : public PlaceholderFilter {
public:
    [[nodiscard]] auto filterNames() const -> text::StringList override;
    [[nodiscard]] auto apply(const text::String &filterName, const text::String &parameter, const text::String &value)
        -> text::String override;

private:
    /// Apply a lower- or upper-case conversion.
    [[nodiscard]] static auto applyCase(
        const text::String &filterName, const text::String &parameter, const text::String &value) -> text::String;
};

}
