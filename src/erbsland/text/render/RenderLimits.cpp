// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "RenderLimits.hpp"

#include "../Literals.hpp"

#include "../../err/ParameterError.hpp"

namespace erbsland::text::render {

using namespace literals;

auto RenderLimits::set(uint64_t &field, const uint64_t value) -> RenderLimits & {
    if (value == 0U) {
        throw err::ParameterError{"A rendering limit must be positive."_el, "value"_el};
    }
    field = value;
    return *this;
}

auto RenderLimits::setGeneratedOutputBytes(const uint64_t value) -> RenderLimits & {
    return set(_generatedOutputBytes, value);
}
auto RenderLimits::setExecutedInstructions(const uint64_t value) -> RenderLimits & {
    return set(_executedInstructions, value);
}
auto RenderLimits::setNestingDepth(const uint64_t value) -> RenderLimits & {
    return set(_nestingDepth, value);
}
auto RenderLimits::setCallbackDepth(const uint64_t value) -> RenderLimits & {
    return set(_callbackDepth, value);
}
auto RenderLimits::setLexicalScopeDepth(const uint64_t value) -> RenderLimits & {
    return set(_lexicalScopeDepth, value);
}
auto RenderLimits::setCallFrameDepth(const uint64_t value) -> RenderLimits & {
    return set(_callFrameDepth, value);
}
auto RenderLimits::setValueStackDepth(const uint64_t value) -> RenderLimits & {
    return set(_valueStackDepth, value);
}
auto RenderLimits::setStaticDependencyDepth(const uint64_t value) -> RenderLimits & {
    return set(_staticDependencyDepth, value);
}

}
