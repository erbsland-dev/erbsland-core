// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "BlockStringBuilder.hpp"

#include "../BlockPrintContext.hpp"
#include "../BlockStringEditor.hpp"
#include "../CursorWriter.hpp"

namespace erbsland::cterm::impl {

/// A print context that builds one resolved block string before committing it to a target.
/// @tested{CursorWriterTest BlockStringTest}
class BlockPrintContextToString : public BlockPrintContext {
public:
    /// Create a new print context with an initial style.
    explicit BlockPrintContextToString(BlockStyle style = {}, bool normalizeInheritedColorArguments = false) noexcept;

    // defaults / prevent copy and move
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

/// A print context that commits a built block string to a cursor writer.
/// @tested{CursorWriterTest}
class BlockPrintContextToCursorWriter final : public BlockPrintContextToString {
public:
    /// Create a new print context for a cursor writer.
    explicit BlockPrintContextToCursorWriter(CursorWriter &writer) noexcept;

    // defaults / prevent copy and move
    ~BlockPrintContextToCursorWriter() override = default;
    BlockPrintContextToCursorWriter(const BlockPrintContextToCursorWriter &) = delete;
    BlockPrintContextToCursorWriter(BlockPrintContextToCursorWriter &&) = delete;
    auto operator=(const BlockPrintContextToCursorWriter &) -> BlockPrintContextToCursorWriter & = delete;
    auto operator=(BlockPrintContextToCursorWriter &&) -> BlockPrintContextToCursorWriter & = delete;

public:
    void commit() noexcept override;

private:
    CursorWriter &_writer;
};

/// A print context that commits a built block string to another block string.
/// @tested{BlockStringTest}
class BlockPrintContextToBlockString final : public BlockPrintContextToString {
public:
    /// Create a new print context for a block string.
    explicit BlockPrintContextToBlockString(BlockStringEditor &text) noexcept;

    // defaults / prevent copy and move
    ~BlockPrintContextToBlockString() override = default;
    BlockPrintContextToBlockString(const BlockPrintContextToBlockString &) = delete;
    BlockPrintContextToBlockString(BlockPrintContextToBlockString &&) = delete;
    auto operator=(const BlockPrintContextToBlockString &) -> BlockPrintContextToBlockString & = delete;
    auto operator=(BlockPrintContextToBlockString &&) -> BlockPrintContextToBlockString & = delete;

public:
    void commit() noexcept override;

private:
    BlockStringEditor &_text;
};

}
