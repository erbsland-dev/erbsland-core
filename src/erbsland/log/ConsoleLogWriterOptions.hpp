// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "LogLevel.hpp"

#include "line/LogLinePart.hpp"

#include "../cterm/BlockStyle.hpp"
#include "../cterm/ParagraphOptions.hpp"

#include <array>

namespace erbsland::log {

/// Style and wrapping settings for console log output.
/// @tested{ConsoleLogWriterTest}
class ConsoleLogWriterOptions final {
public:
    /// Create default console styling and paragraph settings.
    ConsoleLogWriterOptions();

    /// Access the paragraph layout settings.
    [[nodiscard]] auto paragraphOptions() const noexcept -> const cterm::ParagraphOptions & { return _paragraph; }
    /// Set the paragraph layout settings.
    /// @param value The complete paragraph options passed to the terminal.
    /// @return These options for chained configuration.
    auto setParagraphOptions(cterm::ParagraphOptions value) noexcept -> ConsoleLogWriterOptions &;
    /// Get the base style applied to each line.
    [[nodiscard]] auto baseLineStyle() const noexcept -> cterm::BlockStyle { return _lineBase; }
    /// Set the base style applied to each line.
    /// @param style The style applied before level and semantic-part styles.
    /// @return These options for chained configuration.
    auto setBaseLineStyle(cterm::BlockStyle style) noexcept -> ConsoleLogWriterOptions &;
    /// Get the additional line style for a severity level.
    /// @param level The severity whose line style shall be returned.
    /// @return The additional style applied to complete lines of `level`.
    [[nodiscard]] auto lineStyle(LogLevel level) const noexcept -> cterm::BlockStyle;
    /// Set the additional line style for a severity level.
    /// @param level The severity whose line style shall be changed.
    /// @param style The additional style applied after the base line style.
    /// @return These options for chained configuration.
    auto setLineStyle(LogLevel level, cterm::BlockStyle style) noexcept -> ConsoleLogWriterOptions &;
    /// Get the effective semantic-part style for a severity level.
    /// @param part The semantic line part whose style shall be resolved.
    /// @param level The entry severity whose overrides shall be included.
    /// @return The merged base-part and part-level style.
    [[nodiscard]] auto partStyle(LogLinePart part, LogLevel level) const noexcept -> cterm::BlockStyle;
    /// Set the base style for a semantic line part.
    /// @param part The semantic line part whose base style shall be changed.
    /// @param style The style applied after complete-line styles.
    /// @return These options for chained configuration.
    auto setPartStyle(LogLinePart part, cterm::BlockStyle style) noexcept -> ConsoleLogWriterOptions &;
    /// Set the additional style for a semantic part and severity level.
    /// @param part The semantic line part whose level override shall be changed.
    /// @param level The severity selecting the override.
    /// @param style The final style applied after all base styles.
    /// @return These options for chained configuration.
    auto setPartStyle(LogLinePart part, LogLevel level, cterm::BlockStyle style) noexcept -> ConsoleLogWriterOptions &;

private:
    /// Convert a severity level into the internal style-array index.
    /// @param level The severity to convert.
    /// @return The bounded array index for the severity.
    [[nodiscard]] static auto levelIndex(LogLevel level) noexcept -> std::size_t;
    /// Convert a semantic line part into the internal style-array index.
    /// @param part The semantic part to convert.
    /// @return The bounded array index for the semantic part.
    [[nodiscard]] static auto partIndex(LogLinePart part) noexcept -> std::size_t;

private:
    cterm::ParagraphOptions _paragraph;                            ///< Terminal paragraph layout settings.
    cterm::BlockStyle _lineBase;                                   ///< Base style applied to every complete line.
    std::array<cterm::BlockStyle, 4U> _lineLevels;                 ///< Additional complete-line styles by level.
    std::array<cterm::BlockStyle, 5U> _partBase;                   ///< Base styles by semantic line part.
    std::array<std::array<cterm::BlockStyle, 4U>, 5U> _partLevels; ///< Final part overrides by part and level.
};

}
