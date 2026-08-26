// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "LayoutGraphCompiler.hpp"

#include "ValidationDigest.hpp"

#include <erbsland/core/ApplicationError.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/render/impl/CompiledBlock.hpp>
#include <erbsland/text/render/impl/CompiledExtends.hpp>
#include <erbsland/text/render/impl/CompiledInclude.hpp>
#include <erbsland/text/render/impl/CompiledLayout.hpp>
#include <erbsland/text/render/impl/Compiler.hpp>

#include <algorithm>

namespace app::render {

namespace el = erbsland;
namespace ri = erbsland::text::render::impl;

using namespace el::text::literals;

auto LayoutGraphCompiler::compile(
    const PreparedLayout &layout, const el::StringMap<el::text::render::FilterFn> &filters)
    -> ri::ConstCompiledLayoutPtr {
    auto compiler = LayoutGraphCompiler{layout, filters};
    return compiler.compile(layout.id, false);
}

auto LayoutGraphCompiler::validate(const ri::ConstCompiledLayoutPtr &root) -> CompiledGraphValidation {
    auto result = CompiledGraphValidation{};
    auto seen = el::StringMap<bool>{};
    validate(root, seen, result);
    return result;
}

auto LayoutGraphCompiler::compile(const el::String &name, const bool ignoreMissing) -> ri::ConstCompiledLayoutPtr {
    if (const auto memoized = _memo.get(name); memoized.has_value()) {
        return *memoized;
    }
    const auto source = _layout.sources.get(name);
    if (!source.has_value()) {
        if (ignoreMissing) {
            return {};
        }
        throw el::ApplicationError{"The profiling layout graph contains a missing required dependency."_el};
    }
    if (_stack.size() > 32U || std::ranges::find(_stack, name) != _stack.end()) {
        throw el::ApplicationError{"The profiling layout graph contains a cycle or excessive depth."_el};
    }
    _stack.emplace_back(name);
    const auto resolver = [&](const el::String &dependency, const bool ignore) -> ri::ConstCompiledLayoutPtr {
        return compile(dependency, ignore);
    };
    try {
        const auto compiled = ri::Compiler{name, **source, _layout.options, _filters, resolver}.compile();
        _memo.set(name, compiled);
        _stack.pop_back();
        return compiled;
    } catch (...) {
        _stack.pop_back();
        throw;
    }
}

void LayoutGraphCompiler::validate(
    const ri::ConstCompiledLayoutPtr &layout, el::StringMap<bool> &seen, CompiledGraphValidation &result) {
    if (layout == nullptr || seen.contains(layout->name())) {
        return;
    }
    seen.set(layout->name(), true);
    ++result.layouts;
    result.bytecodeBytes +=
        layout->setupProgram().data().length().toRawValue() + layout->bodyProgram().data().length().toRawValue();
    for (const auto &entry : layout->blocks()) {
        result.bytecodeBytes += entry.second->program().data().length().toRawValue();
        ++result.blockPrograms;
    }
    result.includes += layout->includes().size();
    if (layout->extendsDependency() != nullptr) {
        ++result.inheritanceEdges;
    }
    result.digest = ValidationDigest::compiled(result.digest, *layout);
    if (layout->extendsDependency() != nullptr) {
        validate(layout->extendsDependency()->layout(), seen, result);
    }
    for (const auto &dependency : layout->includes()) {
        validate(dependency->layout(), seen, result);
    }
}

}
