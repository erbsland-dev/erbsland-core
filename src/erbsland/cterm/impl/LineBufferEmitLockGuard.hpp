// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "LineBuffer_fwd.hpp"

namespace erbsland::cterm::impl {

/// Temporarily suppress automatic emission from a line buffer.
class LineBufferEmitLockGuard {
public:
    /// Suppress automatic emission for the given line buffer.
    /// @param lineBuffer The buffer whose emission is suppressed.
    explicit LineBufferEmitLockGuard(LineBuffer &lineBuffer);
    /// Restore automatic emission for the line buffer.
    ~LineBufferEmitLockGuard();

    // defaults/deletions
    LineBufferEmitLockGuard(const LineBufferEmitLockGuard &) = delete;
    LineBufferEmitLockGuard(LineBufferEmitLockGuard &&) = delete;
    auto operator=(const LineBufferEmitLockGuard &) -> LineBufferEmitLockGuard & = delete;
    auto operator=(LineBufferEmitLockGuard &&) -> LineBufferEmitLockGuard & = delete;

private:
    LineBuffer &_lineBuffer;
};

}
