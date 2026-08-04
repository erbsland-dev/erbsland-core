// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "BlockStringBuilder.hpp"

#include "../BlockPrintContext.hpp"
#include "../BlockStringEditor.hpp"

namespace erbsland::cterm::impl {

/// A print context that builds one resolved block string before committing it to a target.
/// @tested{CursorWriterTest BlockStringTest}
class BlockPrintContextToString : public BlockPrintContext {
public:
    /// Create a new print context with an initial style.
    explicit BlockPrintContextToString(BlockStyle style = {}, bool normalizeInheritedColorArguments = false) noexcept;

    // defaults/deletions
    ~BlockPrintContextToString() override = default;
    BlockPrintContextToString(const BlockPrintContextToString &) = delete;
    BlockPrintContextToString(BlockPrintContextToString &&) = delete;
    auto operator=(const BlockPrintContextToString &) -> BlockPrintContextToString & = delete;
    auto operator=(BlockPrintContextToString &&) -> BlockPrintContextToString & = delete;

public:
    using BlockPrintContext::print;

    void print(Color color) noexcept override;
    void print(Foreground color) noexcept override;
    void print(Background color) noexcept override;
    void print(BlockStyle style) noexcept override;
    void print(BlockAttributes attributes) noexcept override;
    void print(const Block &character) noexcept override;
    void print(const BlockStringEditor &text) noexcept override;
    void print(const BlockString &text) noexcept override;
    void print(const text::String &text) noexcept override;
    void print(const text::U32String &text) noexcept override;

protected:
    BlockStringBuilder _builder;
    BlockStyle _style{};
    bool _normalizeInheritedColorArguments{false};
};

}
