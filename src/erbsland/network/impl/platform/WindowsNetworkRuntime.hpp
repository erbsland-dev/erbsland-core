// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <memory>

namespace erbsland::network::impl {

/// Shared process-wide Winsock runtime.
/// @notest{A process can initialize and clean up Winsock only on Windows.}
class WindowsNetworkRuntime final {
public:
    /// Clean up the process-wide Winsock runtime.
    ~WindowsNetworkRuntime();

    // defaults/deletions
    WindowsNetworkRuntime(const WindowsNetworkRuntime &) = delete;
    auto operator=(const WindowsNetworkRuntime &) -> WindowsNetworkRuntime & = delete;

public:
    /// Acquire the shared Winsock runtime.
    /// @return A shared runtime reference that keeps Winsock initialized.
    [[nodiscard]] static auto shared() -> std::shared_ptr<WindowsNetworkRuntime>;

private:
    /// Initialize the process-wide Winsock runtime.
    WindowsNetworkRuntime();
};

}
