// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::text::render {

/// Limits protecting a rendering operation from excessive resource use.
/// @tested{RenderLanguageCompletionTest}
class RenderLimits final {
public:
    // defaults
    RenderLimits() = default;

public: // accessors
    /// Get the maximum combined generated and captured output in bytes.
    [[nodiscard]] auto generatedOutputBytes() const noexcept -> uint64_t { return _generatedOutputBytes; }
    /// Set the maximum combined generated and captured output in bytes.
    auto setGeneratedOutputBytes(uint64_t value) -> RenderLimits &;
    /// Get the maximum number of executed instructions.
    [[nodiscard]] auto executedInstructions() const noexcept -> uint64_t { return _executedInstructions; }
    /// Set the maximum number of executed instructions.
    auto setExecutedInstructions(uint64_t value) -> RenderLimits &;
    /// Get the maximum runtime nesting depth.
    [[nodiscard]] auto nestingDepth() const noexcept -> uint64_t { return _nestingDepth; }
    /// Set the maximum runtime nesting depth.
    auto setNestingDepth(uint64_t value) -> RenderLimits &;
    /// Get the maximum callback resolution depth.
    [[nodiscard]] auto callbackDepth() const noexcept -> uint64_t { return _callbackDepth; }
    /// Set the maximum callback resolution depth.
    auto setCallbackDepth(uint64_t value) -> RenderLimits &;
    /// Get the maximum lexical scope depth.
    [[nodiscard]] auto lexicalScopeDepth() const noexcept -> uint64_t { return _lexicalScopeDepth; }
    /// Set the maximum lexical scope depth.
    auto setLexicalScopeDepth(uint64_t value) -> RenderLimits &;
    /// Get the maximum call-frame depth.
    [[nodiscard]] auto callFrameDepth() const noexcept -> uint64_t { return _callFrameDepth; }
    /// Set the maximum call-frame depth.
    auto setCallFrameDepth(uint64_t value) -> RenderLimits &;
    /// Get the maximum value-stack depth.
    [[nodiscard]] auto valueStackDepth() const noexcept -> uint64_t { return _valueStackDepth; }
    /// Set the maximum value-stack depth.
    auto setValueStackDepth(uint64_t value) -> RenderLimits &;
    /// Get the maximum static dependency depth.
    [[nodiscard]] auto staticDependencyDepth() const noexcept -> uint64_t { return _staticDependencyDepth; }
    /// Set the maximum static dependency depth.
    auto setStaticDependencyDepth(uint64_t value) -> RenderLimits &;

private:
    /// Validate and replace one positive limit field.
    auto set(uint64_t &field, uint64_t value) -> RenderLimits &;

private:
    uint64_t _generatedOutputBytes{64U * 1024U * 1024U};
    uint64_t _executedInstructions{10'000'000U};
    uint64_t _nestingDepth{128U};
    uint64_t _callbackDepth{32U};
    uint64_t _lexicalScopeDepth{128U};
    uint64_t _callFrameDepth{128U};
    uint64_t _valueStackDepth{1'024U};
    uint64_t _staticDependencyDepth{32U};
};

}
