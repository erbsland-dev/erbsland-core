// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CompiledInclude_fwd.hpp"
#include "CompiledLayout_fwd.hpp"

#include "../../../text/String.hpp"
#include "../../../unit/CodeLocation.hpp"

namespace erbsland::text::render::impl {

/// One immutable static-include dependency bound into a compiled generation.
/// @tested{RenderIncludeTest RenderProgramTest}
class CompiledInclude final {
public:
    /// Create a bound or ignored-missing include dependency.
    CompiledInclude(
        String name,
        ConstCompiledLayoutPtr layout,
        bool withContext,
        bool ignoreMissing,
        unit::CodeLocation location) noexcept :
        _name{std::move(name)},
        _layout{std::move(layout)},
        _withContext{withContext},
        _ignoreMissing{ignoreMissing},
        _location{location} {}

    // defaults
    ~CompiledInclude() = default;
    CompiledInclude(const CompiledInclude &) = default;
    CompiledInclude(CompiledInclude &&) noexcept = default;
    auto operator=(const CompiledInclude &) -> CompiledInclude & = default;
    auto operator=(CompiledInclude &&) noexcept -> CompiledInclude & = default;

public: // accessors
    /// Access the exact logical dependency name.
    [[nodiscard]] auto name() const noexcept -> const String & { return _name; }
    /// Access the bound generation, or null for an ignored missing layout.
    [[nodiscard]] auto layout() const noexcept -> const ConstCompiledLayoutPtr & { return _layout; }
    /// Test whether the include receives its parent's active context.
    [[nodiscard]] auto withContext() const noexcept -> bool { return _withContext; }
    /// Test whether a missing layout is ignored.
    [[nodiscard]] auto ignoreMissing() const noexcept -> bool { return _ignoreMissing; }
    /// Access the include statement location in the owning layout.
    [[nodiscard]] auto location() const noexcept -> unit::CodeLocation { return _location; }

private:
    String _name;                   ///< Exact logical dependency name.
    ConstCompiledLayoutPtr _layout; ///< Bound immutable generation, if present.
    bool _withContext{true};        ///< Whether parent-visible values are inherited.
    bool _ignoreMissing{false};     ///< Whether absence is accepted.
    unit::CodeLocation _location;   ///< Include statement location in the owning source.
};

}
