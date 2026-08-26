// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ProfileTypes.hpp"

#include <erbsland/text/render/impl/CompiledLayout_fwd.hpp>

#include <vector>

namespace app::render {

/// Compile and measure one preloaded static layout dependency graph.
/// @notest{Covered by the render profiler smoke and determinism tests.}
class LayoutGraphCompiler final {
public:
    /// Compile one fresh immutable graph without loader or cache timing.
    [[nodiscard]] static auto compile(
        const PreparedLayout &layout, const erbsland::StringMap<erbsland::text::render::FilterFn> &filters)
        -> erbsland::text::render::impl::ConstCompiledLayoutPtr;
    /// Measure one immutable graph using each logical layout once.
    [[nodiscard]] static auto validate(const erbsland::text::render::impl::ConstCompiledLayoutPtr &root)
        -> CompiledGraphValidation;

private:
    /// Create one recursive graph compiler.
    /// @param layout The preloaded root and named dependency sources.
    /// @param filters The immutable filter registry used for every graph node.
    LayoutGraphCompiler(
        const PreparedLayout &layout, const erbsland::StringMap<erbsland::text::render::FilterFn> &filters) noexcept :
        _layout{layout}, _filters{filters} {}
    /// Compile one named source recursively.
    [[nodiscard]] auto compile(const erbsland::String &name, bool ignoreMissing)
        -> erbsland::text::render::impl::ConstCompiledLayoutPtr;
    /// Measure one unique generation recursively.
    static void validate(
        const erbsland::text::render::impl::ConstCompiledLayoutPtr &layout,
        erbsland::StringMap<bool> &seen,
        CompiledGraphValidation &result);

private:
    const PreparedLayout &_layout; ///< Preloaded root and named dependency sources.
    const erbsland::StringMap<erbsland::text::render::FilterFn> &_filters;           ///< Identity filter registry.
    erbsland::StringMap<erbsland::text::render::impl::ConstCompiledLayoutPtr> _memo; ///< Compiled unique nodes.
    std::vector<erbsland::String> _stack; ///< Active cycle/depth detection stack.
};

}
