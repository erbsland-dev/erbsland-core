// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ReadLineBase.hpp"

#include "../ReadLine.hpp"

#include "../../text/u32/U32StringList.hpp"
#include "../../unit/ElementIndex.hpp"

namespace erbsland::cterm::impl {

class ReadLineTestAccess;

/// Ordinary terminal line editor with ordinary text, history, and result storage.
/// @tested{ReadLineTest}
class ReadLine final : public ReadLineBase, public cterm::ReadLine {
public:
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
    void normalizeConfiguredText();
    [[nodiscard]] auto normalizedText(const text::String &value) const -> text::U32StringEditor;
    [[nodiscard]] auto textAsString() const -> text::String;
    [[nodiscard]] auto logicalLineCount() const noexcept -> std::size_t;
    void loadHistoryEntry(unit::ElementIndex index);
    void appendCommittedHistory();
    [[nodiscard]] auto result(ReadLineStatus status) const -> ReadLineResult;

private:
    text::U32StringList _history;                    ///< Normalized reusable history.
    text::U32StringEditor _initialText;              ///< Normalized starting text.
    text::U32StringEditor _text;                     ///< Active ordinary edit text.
    std::optional<unit::ElementIndex> _historyIndex; ///< Selected history entry.
    text::U32StringEditor _historyDraft;             ///< Draft restored after newest history.
    unit::CpIndex _historyDraftCursor;               ///< Cursor in the saved history draft.
    text::String _committedText;                     ///< Ordinary committed result.
};

}
