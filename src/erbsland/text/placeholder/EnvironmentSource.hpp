// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Source.hpp"

namespace erbsland::text::placeholder {

/// Built-in environment-variable placeholder source.
/// @tested{ReplacerTest ParserPlaceholderTest}
class EnvironmentSource final : public Source {
    /// Parsed environment-source parameters.
    struct Parameters final {
        String variableName;     ///< Platform environment-variable name.
        bool isRequired{false};  ///< Throw if the variable is not set.
        bool isUnsafeRaw{false}; ///< Return the variable without character filtering.
    };

public:
    /// Create the environment source with `env` or a custom registered name.
    explicit EnvironmentSource(const String &name = {});

public:
    [[nodiscard]] auto sourceNames() const -> StringList override;
    [[nodiscard]] auto resolve(const String &sourceName, const String &parameter) -> String override;
    [[nodiscard]] auto validate(const String &sourceName, const String &parameter) -> bool override;

private:
    /// Parse the variable name and optional flags.
    [[nodiscard]] static auto parseParameters(const String &parameter) -> Parameters;
    /// Apply one optional flag.
    static void applyFlag(Parameters &parameters, const String &flag);

private:
    String _name; ///< Registered source name.
};

}
