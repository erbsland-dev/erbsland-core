// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "StandardStreamRedirectData_fwd.hpp"
#include "StandardStreamSlot.hpp"

#include "../TextInputStream.hpp"
#include "../TextOutputStream.hpp"

#include <memory>
#include <mutex>
#include <source_location>
#include <unordered_map>

namespace erbsland::stream::impl {

/// Process-wide registry for standard stream proxies and replacement targets.
/// @tested{StandardStreamsTest}
class StandardStreamRegistry final {
public:
    // defaults
    StandardStreamRegistry() = default;

    ~StandardStreamRegistry() = default;
    StandardStreamRegistry(const StandardStreamRegistry &) = delete;
    auto operator=(const StandardStreamRegistry &) -> StandardStreamRegistry & = delete;
    StandardStreamRegistry(StandardStreamRegistry &&) = delete;
    auto operator=(StandardStreamRegistry &&) -> StandardStreamRegistry & = delete;

public:
    /// Get the stable standard input proxy.
    [[nodiscard]] auto inputProxy() -> TextInputStreamPtr;
    /// Get the stable standard output proxy.
    [[nodiscard]] auto outputProxy() -> TextOutputStreamPtr;
    /// Get the stable standard error proxy.
    [[nodiscard]] auto errorProxy() -> TextOutputStreamPtr;
    /// Get the current standard input target.
    [[nodiscard]] auto inputTarget() -> TextInputStreamPtr;
    /// Get the current standard output target.
    [[nodiscard]] auto outputTarget() -> TextOutputStreamPtr;
    /// Get the current standard error target.
    [[nodiscard]] auto errorTarget() -> TextOutputStreamPtr;
    /// Replace one or both targets.
    [[nodiscard]] auto replace(
        StandardStreamSlot slot, TextInputStreamPtr input, TextOutputStreamPtr output, TextOutputStreamPtr error)
        -> std::shared_ptr<StandardStreamRedirectData>;
    /// Restore one or both targets.
    void restore(
        StandardStreamSlot slot,
        TextInputStreamPtr input,
        TextOutputStreamPtr output,
        TextOutputStreamPtr error) noexcept;
    /// Register one native standard-input sensitivity request.
    [[nodiscard]] auto startSensitiveInput(std::source_location location) -> uint64_t;
    /// Stop one registered native standard-input sensitivity request.
    void stopSensitiveInput(uint64_t id);

private:
    /// Get the native standard-input target while the registry mutex is held.
    [[nodiscard]] auto nativeInputTargetLocked() -> TextInputStreamPtr;

private:
    std::mutex _mutex;                     ///< Synchronizes access to the registry.
    TextInputStreamPtr _inputTarget;       ///< The current input target.
    TextInputStreamPtr _nativeInputTarget; ///< The separately tracked process-native input target.
    TextOutputStreamPtr _outputTarget;     ///< The current output target.
    TextOutputStreamPtr _errorTarget;      ///< The current error target.
    TextInputStreamPtr _inputProxy;        ///< The stable input proxy.
    TextOutputStreamPtr _outputProxy;      ///< The stable output proxy.
    TextOutputStreamPtr _errorProxy;       ///< The stable error proxy.
    uint64_t _nextSensitiveInputId{1U};    ///< Next diagnostic sensitivity identifier.
    std::unordered_map<uint64_t, std::source_location> _sensitiveInputRequests; ///< Active native-input requests.
};

/// Access the process-wide standard stream registry.
[[nodiscard]] auto standardStreamRegistry() -> StandardStreamRegistry &;

}
