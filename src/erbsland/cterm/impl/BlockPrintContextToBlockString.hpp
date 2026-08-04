// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "BlockPrintContextToString.hpp"

#include "../BlockStringEditor.hpp"

namespace erbsland::cterm::impl {

/// A print context that commits a built block string to another block string.
/// @tested{BlockStringTest}
class BlockPrintContextToBlockString final : public BlockPrintContextToString {
public:
    /// Create a new print context for a block string.
    explicit BlockPrintContextToBlockString(BlockStringEditor &text) noexcept;

    // defaults/deletions
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
