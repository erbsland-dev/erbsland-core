// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../BlockPrintContext.hpp"
#include "../Terminal.hpp"

namespace erbsland::cterm::impl {

/// A print context that writes directly to a terminal.
/// @tested{TerminalConvenienceTest}
class BlockPrintContextToTerminal final : public BlockPrintContext {
public:
    /// Create a new print context for a terminal.
    explicit BlockPrintContextToTerminal(Terminal &terminal) noexcept : _terminal{terminal} {}

    // defaults / prevent copy and move
    ~BlockPrintContextToTerminal() override = default;
    BlockPrintContextToTerminal(const BlockPrintContextToTerminal &) = delete;
    BlockPrintContextToTerminal(BlockPrintContextToTerminal &&) = delete;
    auto operator=(const BlockPrintContextToTerminal &) -> BlockPrintContextToTerminal & = delete;
    auto operator=(BlockPrintContextToTerminal &&) -> BlockPrintContextToTerminal & = delete;

public:
    using BlockPrintContext::print;

    void commit() noexcept override {}
    void print(Color color) noexcept override;
    void print(Foreground color) noexcept override;
    void print(Background color) noexcept override;
    void print(BlockStyle style) noexcept override;
    void print(BlockAttributes attributes) noexcept override;
    void print(const Block &character) noexcept override;
    void print(const BlockString &text) noexcept override;
    void print(const BlockStringView &text) noexcept override;
    void print(const text::StringView &text) noexcept override;
    void print(const text::U32StringView &text) noexcept override;

private:
    Terminal &_terminal;
};

}
