// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ReadLineBase.hpp"
#include "ReadLineTestAccess_fwd.hpp"

#include "../ReadLine.hpp"

#include "../../text/u32/U32StringList.hpp"
#include "../../unit/ItemIndex.hpp"

namespace erbsland::cterm::impl {

/// Ordinary terminal line editor with ordinary text, history, and result storage.
/// @tested{ReadLineTest}
class ReadLine final : public ReadLineBase, public cterm::ReadLine {
public:
    /// Create an ordinary terminal line editor.
    /// @param terminal The terminal used for input and display.
    /// @param options The editor options.
    ReadLine(TerminalPtr terminal, ReadLineOptions options);
    ~ReadLine() override;

public: // implement cterm::ReadLine
    void start() override { startBase(); }
    [[nodiscard]] auto update() -> ReadLineResult override;
    [[nodiscard]] auto waitForInput() -> ReadLineResult override;
    void stop() noexcept override { stopBase(); }
    [[nodiscard]] auto isActive() const noexcept -> bool override { return isActiveBase(); }

private:
    friend class ReadLineTestAccess;
    /// Create a line editor with a custom clock for testing.
    /// @param terminal The terminal used for input and display.
    /// @param options The editor options.
    /// @param nowFn The clock used to obtain the current time.
    ReadLine(TerminalPtr terminal, ReadLineOptions options, NowFn nowFn);

private: // implement ReadLineBase storage
    void resetText() override;
    void discardText() noexcept override;
    void commitText() override;
    [[nodiscard]] auto displayText() const noexcept -> const text::U32StringEditor & override { return _text; }
    [[nodiscard]] auto insertKeyText(const Key &key, unit::CpIndex index) -> unit::CpLength override;
    [[nodiscard]] auto insertNewLine(unit::CpIndex index) -> bool override;
    void eraseText(unit::CpIndex index, unit::CpLength length) noexcept override;
    [[nodiscard]] auto previousUnitStart(unit::CpIndex index) const noexcept -> unit::CpIndex override;
    [[nodiscard]] auto nextUnitEnd(unit::CpIndex index) const noexcept -> unit::CpIndex override;
    [[nodiscard]] auto selectPreviousHistory() -> bool override;
    [[nodiscard]] auto selectNextHistory() -> bool override;

private:
    /// Normalize configured history and initial text.
    void normalizeConfiguredText();
    /// Normalize a text value for editing storage.
    [[nodiscard]] auto normalizedText(const text::String &value) const -> text::U32StringEditor;
    /// Convert current editing text to a Core string.
    [[nodiscard]] auto textAsString() const -> text::String;
    /// Get the number of logical lines in current editing text.
    [[nodiscard]] auto logicalLineCount() const noexcept -> std::size_t;
    /// Load one history entry into the editor.
    void loadHistoryEntry(unit::ItemIndex index);
    /// Append committed text to reusable history.
    void appendCommittedHistory();
    /// Build a line-editing result with current text.
    [[nodiscard]] auto result(ReadLineStatus status) const -> ReadLineResult;

private:
    text::U32StringList _history;                 ///< Normalized reusable history.
    text::U32StringEditor _initialText;           ///< Normalized starting text.
    text::U32StringEditor _text;                  ///< Active ordinary edit text.
    std::optional<unit::ItemIndex> _historyIndex; ///< Selected history entry.
    text::U32StringEditor _historyDraft;          ///< Draft restored after newest history.
    unit::CpIndex _historyDraftCursor;            ///< Cursor in the saved history draft.
    text::String _committedText;                  ///< Ordinary committed result.
};

}
