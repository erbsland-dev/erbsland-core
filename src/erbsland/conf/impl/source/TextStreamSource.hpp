// Copyright (c) 2024-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../stream/TextInputStream.hpp"
#include "../../Source.hpp"

#include <deque>
#include <optional>

namespace erbsland::conf::impl {

/// Base class for configuration sources backed by a Core text input stream.
///
/// The five most recently read lines are retained for best-effort error diagnostics.
/// @tested{TextStreamSourceTest FileSourceTest}
class TextStreamSource : public Source {
public:
    // defaults
    TextStreamSource() = default;
    ~TextStreamSource() override = default;

public:
    void open() override;
    [[nodiscard]] auto isOpen() const noexcept -> bool override { return _isOpen; }
    [[nodiscard]] auto atEnd() const noexcept -> bool override { return _atEnd; }
    [[nodiscard]] auto readLine() -> text::String override;
    [[nodiscard]] auto codeSnippet(unit::CodeLocation location) noexcept -> std::optional<text::CodeSnippet> override;
    void close() noexcept override;

protected:
    /// Create and open the Core text input stream.
    /// @throws ConfError If the stream cannot be created or opened.
    [[nodiscard]] virtual auto createStream() -> stream::TextInputStreamPtr = 0;

private:
    /// Read one line from the Core text stream.
    [[nodiscard]] auto readStreamLine() -> std::optional<text::String>;
    /// Retain a recently read source line for diagnostics.
    void rememberLine(const text::String &line);
    /// Mark the source as having reached the end of input.
    void sourceIsAtEnd() noexcept;
    /// Throw an error for a line exceeding the configured length.
    [[noreturn]] void throwLineLengthExceeded();
    /// Throw an error for a stream read timeout.
    [[noreturn]] void throwTimeout();
    /// Throw an error for an unsuccessful stream read.
    [[noreturn]] void throwReadError(ConfErrorCategory category, text::String title, text::String description);

private:
    stream::TextInputStreamPtr _stream;                                ///< The Core text stream.
    std::deque<std::pair<unit::LineIndex, text::String>> _recentLines; ///< The last five input lines.
    unit::LineIndex _nextLine{unit::LineIndex::zero()};                ///< The index assigned to the next line read.
    bool _isOpen{false};                                               ///< If this source is open.
    bool _atEnd{false};                                                ///< If this source reached its end.
};

}
