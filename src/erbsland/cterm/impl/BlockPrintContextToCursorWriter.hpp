// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "BlockPrintContextToString.hpp"

#include "../CursorWriter.hpp"

namespace erbsland::cterm::impl {

/// A print context that commits a built block string to a cursor writer.
/// @tested{CursorWriterTest}
class BlockPrintContextToCursorWriter final : public BlockPrintContextToString {
public:
    /// Create a new print context for a cursor writer.
    explicit BlockPrintContextToCursorWriter(CursorWriter &writer) noexcept;

    // defaults/deletions
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

}
