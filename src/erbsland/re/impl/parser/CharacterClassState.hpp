// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "EscapeSequenceHandler.hpp"

#include <tuple>
#include <utility>

namespace erbsland::re::impl::parser {

/// A state for the character class handler.
/// Acts as a wrapper around the parser state and adds helper methods for an efficient implementation.
class CharClassHandlerState {
public:
    /// Create a new state.
    explicit CharClassHandlerState(ParserState &state) : _state{state} {}

public: // the public interface.
    /// Parse a `[...]` class.
    void parse();
    /// Take the result.
    [[nodiscard]] auto takeResult() && noexcept -> std::tuple<std::vector<CharRange>, std::vector<Category>, bool> {
        return {std::move(_ranges), std::move(_categories), _isNegated};
    };

private: // wrapper around `ParserState`
    /// Test if a parser feature is enabled.
    [[nodiscard]] auto hasFeature(Feature feature) const noexcept -> bool;
    /// Read the next pattern character.
    void readNext();
    /// Test if parsing reached the end of input.
    [[nodiscard]] auto isAtEnd() const noexcept -> bool;
    /// Get the current pattern character.
    [[nodiscard]] auto currentChar() const noexcept -> text::Char;
    /// Get the current group flags.
    [[nodiscard]] auto currentFlags() const noexcept -> GroupFlags;
    /// Throw a parsing error through the owning parser state.
    template <typename Fwd>
    [[noreturn]] auto throwParsingError(Fwd &&message) -> void {
        _state.throwParsingError(std::forward<Fwd>(message));
    }

private: // helper methods
    /// Test if the class already has content.
    [[nodiscard]] auto hasContent() const noexcept -> bool;
    /// Add a single character (as A-A range).
    void addCharacter(text::Char character);
    /// Add a range (A-B).
    void addRange(text::Char start, text::Char end);
    /// Test if there is a start literal.
    [[nodiscard]] auto hasStartLiteral() const noexcept -> bool;
    /// Get the current start literal.
    [[nodiscard]] auto startLiteral() const noexcept -> text::Char;
    /// Set the current start literal.
    void setStartLiteral(text::Char literal) noexcept;
    /// Clear the current start literal.
    void clearStartLiteral() noexcept;
    /// Test if we have a range open.
    [[nodiscard]] auto isRange() const noexcept -> bool;
    /// Set the range flag.
    void setRange(bool isRange) noexcept;
    /// Test if this is a negated character class.
    [[nodiscard]] auto isNegated() const noexcept -> bool;
    /// Set the negated flag.
    void setNegated(bool negated) noexcept;

public: // handler methods
    /// Test if there is more and fail if not.
    void expectMore();
    /// Handle the `^` character in a character class.
    void handleNegation();
    /// Handle the `-` range character in a character class.
    void handleRangeChar();
    /// Handle the `[:name:]` posix character class.
    void handlePosixCharacterClass();
    /// Handle a list of ranges.
    void handleRangeList(const std::vector<CharRange> &ranges);
    /// Handle a Unicode category.
    void addCategory(Category category);
    /// Handle the digit escape.
    void handleDigit();
    /// Handle the word escape.
    void handleWord();
    /// Handle the space escape.
    void handleSpace();
    /// Handle a Unicode property name.
    void handleUnicodePropertyName();
    /// Handle the horizontal space escape.
    void handleHorizontalSpace();
    /// Handle the vertical space escape.
    void handleVerticalSpace();
    /// Handle a single escapes character.
    void handleSingleEscapedCharacter(text::Char character);
    /// Handle a legacy single escapes character.
    void handleLegacySingleEscapedCharacter(text::Char character, Feature feature);
    /// Handle a quoted block of characters \\Q-\\E
    void handleQuotedLiteral();
    /// Throw the exception on negative ranges.
    [[noreturn]] void throwNegationNotAllowed();
    /// Throw an exception on anchors.
    [[noreturn]] void throwAnchorNotAllowed();
    /// Handle any escape sequence starting with a backslash.
    void handleEscapeSequence();
    /// Add the ranges for a "POSIX" character class (PCRE compatibility).
    void processPosixRangesFor(const text::String &name);
    /// Process the start literal before a range.
    void processStartLiteralBeforeRange();
    /// Process a new literal that may be part of a range.
    void processNewLiteral(text::Char character);

private:
    ParserState &_state;                                 ///< A reference to the current parser state.
    std::vector<CharRange> _ranges;                      ///< The collected ranges.
    std::vector<Category> _categories;                   ///< The collected categories.
    bool _isNegated = false;                             ///< If the whole expression is negated.
    text::Char _startLiteral{text::Char::noCodePoint()}; ///< The current start literal, or the no-code-point signal.
    bool _isRange = false;                               ///< If there is a range open (`xxx-...`).
};

}
