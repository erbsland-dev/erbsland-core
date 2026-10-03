// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CursorBuffer.hpp"
#include "TerminalEmulationOptions.hpp"

#include "impl/EmulationCursorState.hpp"

#include "../text/StringDecodeBuffer.hpp"

#include <array>
#include <vector>

namespace erbsland::cterm {

/// Incrementally interpret UTF-8 and ANSI output into fixed terminal frames and scrollback.
/// @seedoc{/reference/cterm/terminal_emulation}
/// @tested{TerminalEmulationTest}
class TerminalEmulation final {
    /// Incremental ANSI parser states, including discarded control-string payloads.
    enum class ParserState { Text, Escape, Csi, StringControl, StringEscape, EscapeIntermediate };

public:
    /// Create an emulator using default options.
    TerminalEmulation();
    /// Create an emulator with an independent copy of its options.
    /// @param options Initial dimensions and behavior.
    explicit TerminalEmulation(TerminalEmulationOptions options);

    // defaults/deletions
    ~TerminalEmulation() = default;
    TerminalEmulation(const TerminalEmulation &) = delete;
    auto operator=(const TerminalEmulation &) -> TerminalEmulation & = delete;

public: // accessors
    /// Test whether output currently targets the alternate screen.
    [[nodiscard]] auto isAlternateScreenActive() const noexcept -> bool { return _alternateActive; }
    /// Get the fixed-size regular terminal frame.
    [[nodiscard]] auto regularBuffer() const noexcept -> const CursorBuffer & { return _regular; }
    /// Get the fixed-size alternate screen, retaining its most recent contents.
    [[nodiscard]] auto alternateBuffer() const noexcept -> const CursorBuffer & { return _alternate; }
    /// Get the screen currently receiving output.
    [[nodiscard]] auto activeBuffer() const noexcept -> const CursorBuffer &;
    /// Get the oldest-to-newest scrollback rows; initially one empty placeholder row.
    [[nodiscard]] auto scrollbackBuffer() const noexcept -> const RemappedBuffer & { return _scrollback; }
    /// Test whether at least one regular-screen row has scrolled into history.
    [[nodiscard]] auto hasScrollback() const noexcept -> bool { return _hasScrollback; }
    /// Get the construction options.
    [[nodiscard]] auto options() const noexcept -> const TerminalEmulationOptions & { return _options; }

public: // input
    /// Process raw UTF-8 bytes, retaining incomplete characters and terminal controls.
    /// @param input Bytes, which need not end at a character or sequence boundary.
    /// Writing after finish() requires reset(). This object is not internally synchronized.
    void write(const text::String &input);
    /// End the stream, replacing incomplete UTF-8 and discarding incomplete terminal controls.
    /// Repeated calls have no effect.
    void finish();
    /// Clear both screens and all parser state, preserving construction options.
    void reset();

private:
    /// Reset screens and control state without discarding already decoded input.
    void resetState();
    /// Append a complete departing regular-screen row, growing or recycling history.
    void appendScrollback(const ReadableBuffer &line) noexcept;
    /// Clear retained history and restore its single empty placeholder row.
    void clearScrollback();
    /// Get mutable access to the active screen.
    auto buffer() noexcept -> CursorBuffer &;
    /// Drain complete decoded characters into the ANSI parser.
    void drain();
    /// Interpret one complete Unicode scalar.
    void consume(text::Char character);
    /// Handle parser-wide cancellation, escape, and C1 controls.
    auto consumeControl(char32_t value) -> bool;
    /// Consume one discarded control-string code point.
    void consumeStringControl(char32_t value);
    /// Interpret one escape command or intermediate prefix.
    void consumeEscape(char32_t value);
    /// Accumulate or complete a CSI sequence.
    void consumeCsi(char32_t value);
    /// Initialize a new CSI sequence.
    void beginCsi();
    /// Apply a printable character or a C0 control.
    void consumeText(text::Char character);
    /// Finish one CSI sequence.
    void executeCsi(char32_t command);
    /// Apply SGR parameters.
    void applyStyle();
    /// Apply one DEC mode.
    void setPrivateMode(int mode, bool enabled);
    /// Erase part or all of the active screen or line.
    void erase(bool entireScreen, int mode);
    /// Advance vertically, optionally returning to column zero.
    void lineFeed();
    /// Save the active cursor and style.
    auto savedState() const -> impl::EmulationCursorState;
    /// Restore an active cursor and style snapshot.
    void restoreState(const impl::EmulationCursorState &state);
    /// Get a parameter, using a default for missing or zero values.
    auto parameter(std::size_t index, int fallback = 1) const -> int;
    /// Index the currently selected screen's saved state.
    auto screenIndex() const noexcept -> std::size_t;

private:
    TerminalEmulationOptions _options; ///< Copied construction options.
    CursorBuffer _regular;             ///< Fixed regular terminal frame.
    CursorBuffer _alternate;           ///< Fixed alternate screen.
    RemappedBuffer _scrollback;        ///< Retained regular-screen rows.
    bool _hasScrollback{false};        ///< The initial placeholder has been replaced.
    text::StringDecodeBuffer _decoder{
        unit::ByteCount{8192},
        text::StringEncoding::Utf8,
        text::StringBomMode::Automatic};                   ///< Incremental UTF-8 decoder.
    ParserState _state{ParserState::Text};                 ///< Current escape parser state.
    std::vector<int> _parameters;                          ///< Bounded CSI parameter values.
    std::array<impl::EmulationCursorState, 2> _decSaved{}; ///< Per-screen DEC cursor snapshots.
    std::array<impl::EmulationCursorState, 2> _scoSaved{}; ///< Per-screen SCO cursor snapshots.
    impl::EmulationCursorState _regularSaved{};            ///< Regular state saved by mode 1049.
    bool _alternateActive{false};                          ///< Active screen selection.
    bool _privateMode{false};                              ///< Current CSI has a DEC private prefix.
    bool _invalidCsi{false};                               ///< Discard the current unsupported or oversized CSI.
    bool _osc{false};                                      ///< BEL can terminate this string control.
    bool _finished{false};                                 ///< Input has been completed.
};

}
