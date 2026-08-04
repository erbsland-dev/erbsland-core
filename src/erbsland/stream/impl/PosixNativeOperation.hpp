// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "PosixNativeStream_fwd.hpp"

namespace erbsland::stream::impl {

/// Keep a POSIX stream file descriptor alive while a native operation runs.
class PosixNativeOperation final {
public:
    /// Begin an operation on the given stream.
    explicit PosixNativeOperation(const PosixNativeStream &stream);
    /// Finish the native operation.
    ~PosixNativeOperation();

    // defaults/deletions
    PosixNativeOperation(const PosixNativeOperation &) = delete;
    PosixNativeOperation(PosixNativeOperation &&) = delete;
    auto operator=(const PosixNativeOperation &) -> PosixNativeOperation & = delete;
    auto operator=(PosixNativeOperation &&) -> PosixNativeOperation & = delete;

public: // accessors
    /// Access the file descriptor retained for the operation.
    [[nodiscard]] auto fileDescriptor() const noexcept -> int { return _fileDescriptor; }

private:
    const PosixNativeStream &_stream;
    int _fileDescriptor{-1};
};

}
