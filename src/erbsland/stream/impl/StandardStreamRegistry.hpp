// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "StandardStreamRedirectData_fwd.hpp"
#include "StandardStreamSlot.hpp"

#include "../TextOutputStream.hpp"

#include <memory>
#include <mutex>

namespace erbsland::stream::impl {

/// Process-wide registry for standard stream proxies and replacement targets.
/// @tested{StandardStreamsTest}
class StandardStreamRegistry final {
public:
    StandardStreamRegistry() = default;

    // defaults
    ~StandardStreamRegistry() = default;
    StandardStreamRegistry(const StandardStreamRegistry &) = delete;
    auto operator=(const StandardStreamRegistry &) -> StandardStreamRegistry & = delete;
    StandardStreamRegistry(StandardStreamRegistry &&) = delete;
    auto operator=(StandardStreamRegistry &&) -> StandardStreamRegistry & = delete;

public:
    /// Get the stable standard output proxy.
    [[nodiscard]] auto outputProxy() -> TextOutputStreamPtr;
    /// Get the stable standard error proxy.
    [[nodiscard]] auto errorProxy() -> TextOutputStreamPtr;
    /// Get the current standard output target.
    [[nodiscard]] auto outputTarget() -> TextOutputStreamPtr;
    /// Get the current standard error target.
    [[nodiscard]] auto errorTarget() -> TextOutputStreamPtr;
    /// Replace one or both targets.
    [[nodiscard]] auto replace(StandardStreamSlot slot, TextOutputStreamPtr output, TextOutputStreamPtr error)
        -> std::shared_ptr<StandardStreamRedirectData>;
    /// Restore one or both targets.
    void restore(StandardStreamSlot slot, TextOutputStreamPtr output, TextOutputStreamPtr error) noexcept;

private:
    std::mutex _mutex;                 ///< Synchronizes access to the registry.
    TextOutputStreamPtr _outputTarget; ///< The current output target.
    TextOutputStreamPtr _errorTarget;  ///< The current error target.
    TextOutputStreamPtr _outputProxy;  ///< The stable output proxy.
    TextOutputStreamPtr _errorProxy;   ///< The stable error proxy.
};

/// Access the process-wide standard stream registry.
[[nodiscard]] auto standardStreamRegistry() -> StandardStreamRegistry &;

}
