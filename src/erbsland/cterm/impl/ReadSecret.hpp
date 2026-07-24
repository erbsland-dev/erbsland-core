// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ReadLineBase.hpp"

#include "../ReadSecret.hpp"

#include <array>

namespace erbsland::cterm::impl {

/// Secret terminal editor with fixed protected character storage and a bullet-only display mask.
/// @tested{ReadSecretTest}
class ReadSecret final : public ReadLineBase, public cterm::ReadSecret {
private:
    struct PreparedOptions final {
        ReadLineOptions options;
        unit::CpLength maximumLength;
    };

public:
    ReadSecret(TerminalPtr terminal, ReadLineOptions options);
    ~ReadSecret() override;

public: // implement cterm::ReadSecret
    void start() override;
    [[nodiscard]] auto update() -> ReadLineResult override;
    [[nodiscard]] auto waitForInput() -> ReadLineResult override;
    void stop() noexcept override;
    [[nodiscard]] auto isActive() const noexcept -> bool override { return isActiveBase(); }

private:
    ReadSecret(TerminalPtr terminal, PreparedOptions prepared);
    [[nodiscard]] static auto prepareOptions(ReadLineOptions options) -> PreparedOptions;

private: // implement ReadLineBase storage
    void resetText() override;
    void discardText() noexcept override;
    void commitText() override;
    [[nodiscard]] auto displayText() const noexcept -> const text::U32StringEditor & override { return _mask; }
    [[nodiscard]] auto insertKeyText(const Key &key, unit::CpIndex index) -> unit::CpLength override;
    [[nodiscard]] auto insertNewLine(unit::CpIndex index) -> bool override;
    void eraseText(unit::CpIndex index, unit::CpLength length) noexcept override;
    [[nodiscard]] auto previousUnitStart(unit::CpIndex index) const noexcept -> unit::CpIndex override;
    [[nodiscard]] auto nextUnitEnd(unit::CpIndex index) const noexcept -> unit::CpIndex override;

private:
    void eraseCharacters(std::size_t begin, std::size_t count) noexcept;
    [[nodiscard]] auto result(ReadLineStatus status) const -> ReadLineResult;
    void purgePendingInput() noexcept;

private:
    std::array<text::Char, 1024U> _characters{}; ///< Fixed protected code-point storage.
    std::size_t _length{};                       ///< Active protected code points.
    unit::CpLength _maximumLength;               ///< Effective capped maximum.
    text::U32StringEditor _mask;                 ///< Ordinary bullet-only display representation.
    text::String _committed;                     ///< Marked committed result.
};

}
