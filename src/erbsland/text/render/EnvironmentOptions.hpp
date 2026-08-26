// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Delimiters.hpp"
#include "RenderLimits.hpp"

#include "../EscapeFormat.hpp"
#include "../Literals.hpp"
#include "../String.hpp"

#include <memory>
#include <utility>
#include <vector>

namespace erbsland::text::render {

/// Options controlling layout syntax.
/// @tested{RenderEnvironmentTest RenderLanguageCompletionTest}
class EnvironmentOptions final {
public:
    /// Create the default environment options.
    EnvironmentOptions();

    // defaults
    EnvironmentOptions(const EnvironmentOptions &) = default;
    EnvironmentOptions(EnvironmentOptions &&) = default;
    auto operator=(const EnvironmentOptions &) -> EnvironmentOptions & = default;
    auto operator=(EnvironmentOptions &&) -> EnvironmentOptions & = default;
    ~EnvironmentOptions() = default;

public:
    /// Get the expression delimiters.
    [[nodiscard]] auto expressionDelimiters() const noexcept -> const Delimiters & { return _expressionDelimiters; }
    /// Set the expression delimiters.
    auto setExpressionDelimiters(const Delimiters &delimiters) noexcept -> EnvironmentOptions & {
        _expressionDelimiters = delimiters;
        return *this;
    }
    /// Get the statement delimiters.
    [[nodiscard]] auto statementDelimiters() const noexcept -> const Delimiters & { return _statementDelimiters; }
    /// Set the statement delimiters.
    auto setStatementDelimiters(const Delimiters &delimiters) noexcept -> EnvironmentOptions & {
        _statementDelimiters = delimiters;
        return *this;
    }
    /// Get the comment delimiters.
    [[nodiscard]] auto commentDelimiters() const noexcept -> const Delimiters & { return _commentDelimiters; }
    /// Set the comment delimiters.
    auto setCommentDelimiters(const Delimiters &delimiters) noexcept -> EnvironmentOptions & {
        _commentDelimiters = delimiters;
        return *this;
    }
    /// Test if automatic escaping is enabled.
    [[nodiscard]] auto automaticEscapingEnabled() const noexcept -> bool { return _automaticEscapingEnabled; }
    /// Enable or disable automatic escaping.
    auto setAutomaticEscapingEnabled(bool enabled) noexcept -> EnvironmentOptions &;
    /// Assign an escape format to a layout-name suffix.
    auto setEscapeFormatForSuffix(const String &suffix, EscapeFormat format) -> EnvironmentOptions &;
    /// Remove the escape format assigned to a suffix.
    auto removeEscapeFormatForSuffix(const String &suffix) -> bool;
    /// Remove all suffix escape-format assignments.
    void clearEscapeFormats();
    /// Look up the escape format for a logical layout name using longest-suffix matching.
    [[nodiscard]] auto escapeFormatForLayout(const String &layoutName) const noexcept -> EscapeFormat;
    /// Access the rendering limits.
    [[nodiscard]] auto renderLimits() const noexcept -> const RenderLimits & { return _renderLimits; }
    /// Replace the rendering limits.
    auto setRenderLimits(const RenderLimits &limits) noexcept -> EnvironmentOptions &;

private:
    using EscapeFormats = std::vector<std::pair<String, EscapeFormat>>;

    /// Access the shared default suffix mappings.
    [[nodiscard]] static auto defaultEscapeFormats() -> const std::shared_ptr<EscapeFormats> &;
    /// Detach shared suffix mappings before modification.
    void detachEscapeFormats();
    /// Validate and normalize one suffix mapping key.
    [[nodiscard]] static auto normalizedSuffix(const String &suffix) -> String;

private:
    Delimiters _expressionDelimiters{text::StringLiteral{"{{"}, text::StringLiteral{"}}"}};
    Delimiters _statementDelimiters{text::StringLiteral{"{%"}, text::StringLiteral{"%}"}};
    Delimiters _commentDelimiters{text::StringLiteral{"{#"}, text::StringLiteral{"#}"}};
    bool _automaticEscapingEnabled{true};
    std::shared_ptr<EscapeFormats> _escapeFormats;
    RenderLimits _renderLimits;
};

}
