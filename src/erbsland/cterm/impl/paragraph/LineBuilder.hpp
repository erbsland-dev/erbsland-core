// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "LayoutContext.hpp"
#include "LayoutPreparedSourceLine.hpp"

#include <optional>
#include <utility>

namespace erbsland::cterm::impl::paragraph {

/// Builds one physical line from a prepared source line and the current layout state.
class LineBuilder final {
    /// The incremental layout state between physical lines of one source line.
    struct State final {
        std::size_t tokenIndex = 0;   ///< The currently processed token index.
        BlockCount wordOffset{};      ///< The character offset inside the current word token.
        std::size_t tabStopIndex = 0; ///< The next configured tab stop index.
    };

    /// The evaluated spacing run before the next word token.
    class SpacingRun final {
    public:
        /// The action to take after evaluating the spacing tokens.
        enum class Action : uint8_t {
            Continue,   ///< Continue with the next word token.
            LineBreak,  ///< Break before the next word token.
            EndOfTokens ///< No word token follows the spacing run.
        };

        /// Create one evaluated spacing run.
        /// @param width The rendered width of the spacing run on this line.
        /// @param nextTokenIndex The next word token, or the next token after a forced break.
        /// @param nextTabStopIndex The next tab stop index after consuming the run.
        /// @param action The selected action for the run.
        SpacingRun(
            const int width = 0,
            const std::size_t nextTokenIndex = 0,
            const std::size_t nextTabStopIndex = 0,
            const Action action = Action::Continue) noexcept :
            width{width}, nextTokenIndex{nextTokenIndex}, nextTabStopIndex{nextTabStopIndex}, action{action} {}

    public:
        int width = 0;                    ///< The rendered width of the spacing run on this line.
        std::size_t nextTokenIndex = 0;   ///< The next word token, or the next token after a forced break.
        std::size_t nextTabStopIndex = 0; ///< The next tab stop index after consuming the run.
        Action action = Action::Continue; ///< The selected action for the run.
    };

    /// The result of building one physical line and the state for the next one.
    class BuildResult final {
    public:
        BuildResult() = default;

        /// Create one build result.
        /// @param line The rendered line.
        /// @param nextState The layout state after consuming the line content.
        /// @param consumedAll `true` if the entire source line has been consumed.
        BuildResult(LayoutLine line, State nextState, const bool consumedAll) :
            line{std::move(line)}, nextState{nextState}, consumedAll{consumedAll} {}

    public:
        LayoutLine line;          ///< The rendered line.
        State nextState;          ///< The layout state after consuming the line content.
        bool consumedAll = false; ///< `true` if the entire source line has been consumed.
    };

public:
    /// Create a line builder for one prepared source line.
    /// @param context The immutable line-build context.
    /// @param preparedSourceLine The prepared source line to render.
    LineBuilder(const LayoutContext &context, const LayoutPreparedSourceLine &preparedSourceLine) noexcept;

    // defaults/deletions
    ~LineBuilder() = default;
    LineBuilder(const LineBuilder &) = delete;
    LineBuilder(LineBuilder &&) = delete;
    auto operator=(const LineBuilder &) -> LineBuilder & = delete;
    auto operator=(LineBuilder &&) -> LineBuilder & = delete;

public:
    /// Build and append all physical lines for the prepared source line.
    /// @param lines The destination vector that receives the produced physical lines.
    /// @return `true` on success, `false` if line construction fails.
    auto appendLinesTo(std::vector<LayoutLine> &lines) -> bool;

private:
    /// Try to build the final physical line for the current builder state.
    /// @return The built line and next state, or `std::nullopt` if more than one physical line is still required.
    [[nodiscard]] auto tryBuildLastLine() const -> std::optional<BuildResult>;
    /// Build a wrapped physical line for the current builder state.
    /// @return The built line and next state, or `std::nullopt` if no wrapped line can be built.
    [[nodiscard]] auto buildWrappedLine() const -> std::optional<BuildResult>;
    /// Build a truncated physical line for the current builder state.
    /// @return The built truncated line and next state, or `std::nullopt` if no truncated line can be built.
    [[nodiscard]] auto buildTruncatedLine() const -> std::optional<BuildResult>;
    /// Build one physical line for the current builder state.
    /// @param reservedSuffixWidth Width reserved for a trailing mark aligned to the right edge.
    /// @param addEndMark `true` if the line should be marked as wrapping to the next line.
    /// @param addEllipsis `true` if the paragraph ellipsis should be appended.
    /// @return The built physical line, or `std::nullopt` if the line cannot be built.
    [[nodiscard]] auto buildLine(int reservedSuffixWidth, bool addEndMark, bool addEllipsis) const
        -> std::optional<BuildResult>;
    [[nodiscard]] auto evaluateSpacingRun(
        std::size_t tokenIndex, std::size_t tabStopIndex, int currentColumn, bool isLineStart) const -> SpacingRun;
    void appendSpacingRun(
        LayoutFragments &fragments,
        std::size_t startTokenIndex,
        std::size_t endTokenIndex,
        std::size_t tabStopIndex,
        int currentColumn,
        bool isLineStart) const;

private:
    const LayoutContext &_context;
    const LayoutPreparedSourceLine &_preparedSourceLine;
    State _state{};
    int _wrappedLineCount = 0;
    bool _isFirstPhysicalLine = true;
};

}
