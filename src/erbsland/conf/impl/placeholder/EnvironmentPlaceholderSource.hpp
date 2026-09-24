// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../PlaceholderSource.hpp"

namespace erbsland::conf::impl::placeholder {

/// Built-in environment-variable placeholder source.
/// @tested{ParserPlaceholderTest}
class EnvironmentPlaceholderSource final : public PlaceholderSource {
    /// Parsed environment-source parameters.
    struct Parameters final {
        text::String variableName; ///< Platform environment-variable name.
        bool isRequired{false};    ///< Throw if the variable is not set.
        bool isUnsafeRaw{false};   ///< Return the variable without character filtering.
    };

public:
    [[nodiscard]] auto sourceNames() const -> text::StringList override;
    [[nodiscard]] auto resolve(const text::String &sourceName, const text::String &parameter) -> text::String override;

private:
    /// Parse the variable name and optional flags.
    [[nodiscard]] static auto parseParameters(const text::String &parameter) -> Parameters;
    /// Apply one optional flag.
    static void applyFlag(Parameters &parameters, const text::String &flag);
};

}
