// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "BlockStyle.hpp"
#include "Terminal_fwd.hpp"
#include "TerminalStream_fwd.hpp"
#include "TerminalStreamSynchronization.hpp"

#include "../stream/TextOutputStream.hpp"

#include <utility>

namespace erbsland::cterm {

/// A text output stream that writes to a terminal with a fixed base style.
/// @tested{TerminalStreamTest}
class TerminalStream final : public stream::TextOutputStream {
public:
    /// Create a terminal stream.
    /// @param terminal The terminal to write to.
    /// @param style The style applied to every write.
    /// @param synchronization Shared synchronization state for streams using the same terminal.
    explicit TerminalStream(
        TerminalPtr terminal,
        BlockStyle style = BlockStyle::reset(),
        TerminalStreamSynchronizationPtr synchronization = {});

    // defaults
    ~TerminalStream() override = default;
    TerminalStream(const TerminalStream &) = delete;
    TerminalStream(TerminalStream &&) = delete;
    auto operator=(const TerminalStream &) -> TerminalStream & = delete;
    auto operator=(TerminalStream &&) -> TerminalStream & = delete;

public:
    /// Create shared synchronization state.
    [[nodiscard]] static auto createSynchronization() -> TerminalStreamSynchronizationPtr;
    /// Create a shared terminal stream.
    [[nodiscard]] static auto create(
        TerminalPtr terminal,
        BlockStyle style = BlockStyle::reset(),
        TerminalStreamSynchronizationPtr synchronization = {}) -> TerminalStreamPtr;
    /// Create synchronized output and error streams for a terminal.
    [[nodiscard]] static auto createStandardStreams(TerminalPtr terminal)
        -> std::pair<TerminalStreamPtr, TerminalStreamPtr>;

public: // implement TextOutputStream
    [[nodiscard]] auto encoding() const noexcept -> text::StringEncoding override;
    [[nodiscard]] auto effectiveEncoding() const noexcept -> text::StringEncoding override;
    [[nodiscard]] auto isOpen() const noexcept -> bool override;
    void flush() override;
    void close() override;
    void write(text::Char character) override;
    void write(const text::StringView &text) override;
    void writeLine() override;
    void writeLine(const text::StringView &text) override;

public: // accessors
    /// Get the terminal used by this stream.
    [[nodiscard]] auto terminal() const noexcept -> const TerminalPtr & { return _terminal; }
    /// Get the style applied to every write.
    [[nodiscard]] auto style() const noexcept -> BlockStyle { return _style; }
    /// Set the style applied to every write.
    void setStyle(const BlockStyle style) noexcept { _style = style; }

private:
    [[nodiscard]] auto requireTerminal() const -> TerminalPtr;
    void resetTerminalStyle(Terminal &terminal);

private:
    TerminalPtr _terminal;                             ///< The terminal to write to.
    BlockStyle _style;                                 ///< The style applied to every write.
    TerminalStreamSynchronizationPtr _synchronization; ///< Shared synchronization state.
};

}
