// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CompiledExtends_fwd.hpp"
#include "CompiledLayout_fwd.hpp"

#include "../../../text/String.hpp"
#include "../../../unit/CodeLocation.hpp"

namespace erbsland::text::render::impl {

/// One immutable static inheritance dependency.
/// @tested{RenderInheritanceTest}
class CompiledExtends final {
public:
    /// Create a bound inheritance dependency.
    CompiledExtends(String name, ConstCompiledLayoutPtr layout, unit::CodeLocation location) noexcept :
        _name{std::move(name)}, _layout{std::move(layout)}, _location{location} {}

    // defaults
    ~CompiledExtends() = default;
    CompiledExtends(const CompiledExtends &) = default;
    CompiledExtends(CompiledExtends &&) noexcept = default;
    auto operator=(const CompiledExtends &) -> CompiledExtends & = default;
    auto operator=(CompiledExtends &&) noexcept -> CompiledExtends & = default;

public: // accessors
    /// Access the exact logical parent name.
    [[nodiscard]] auto name() const noexcept -> const String & { return _name; }
    /// Access the bound immutable parent generation.
    [[nodiscard]] auto layout() const noexcept -> const ConstCompiledLayoutPtr & { return _layout; }
    /// Access the extends statement location.
    [[nodiscard]] auto location() const noexcept -> unit::CodeLocation { return _location; }

private:
    String _name;                   ///< Exact logical parent name.
    ConstCompiledLayoutPtr _layout; ///< Bound immutable parent generation.
    unit::CodeLocation _location;   ///< Extends statement location.
};

}
