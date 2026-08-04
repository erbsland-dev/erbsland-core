// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "WindowsNativeStream_fwd.hpp"

namespace erbsland::stream::impl {

/// Keep a native handle alive while a Windows stream operation runs.
class WindowsNativeOperation final {
public:
    /// Begin an operation on the given stream.
    explicit WindowsNativeOperation(const WindowsNativeStream &stream);
    /// Finish the native operation.
    ~WindowsNativeOperation();

    // defaults/deletions
    WindowsNativeOperation(const WindowsNativeOperation &) = delete;
    WindowsNativeOperation(WindowsNativeOperation &&) = delete;
    auto operator=(const WindowsNativeOperation &) -> WindowsNativeOperation & = delete;
    auto operator=(WindowsNativeOperation &&) -> WindowsNativeOperation & = delete;

public: // accessors
    /// Access the handle retained for the operation.
    [[nodiscard]] auto handle() const noexcept -> WindowsNativeHandle { return _handle; }

private:
    const WindowsNativeStream &_stream;
    WindowsNativeHandle _handle{};
    WindowsNativeHandle _threadHandle{};
};

}
