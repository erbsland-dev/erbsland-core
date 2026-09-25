// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../PackageSettings.hpp"

#include <erbsland/text/placeholder/Source.hpp>

namespace erbsland::package::impl {

/// Resolves the package tool's distinct `%{...}` format.
/// @notest{Covered by package integration tests.}
class PackagePlaceholderSource final : public text::placeholder::Source {
public:
    /// Bind the effective package and current target context.
    PackagePlaceholderSource(
        const PackageSettings &settings, const text::String &platform, const text::String &architecture,
        const text::String &target);
    /// List supported placeholder sources.
    [[nodiscard]] auto sourceNames() const -> text::StringList override;
    /// Resolve one package placeholder.
    [[nodiscard]] auto resolve(const text::String &source, const text::String &parameter) -> text::String override;

private:
    const PackageSettings &_settings; ///< Effective settings for this expansion.
    text::String _platform;            ///< Platform selector.
    text::String _architecture;        ///< Target architecture selector.
    text::String _target;              ///< Optional CMake target name.
};

}
