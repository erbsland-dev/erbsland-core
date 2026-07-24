// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../core/Definitions.hpp"

#include <cstdint>
#include <optional>
#include <source_location>

namespace erbsland::stream::io {

/// A diagnostic handle for one active native standard-input sensitivity request.
/// The token is move-only and does not stop the request when destroyed. Use `SensitiveInputScope` for RAII.
/// @tested{StandardStreamsTest}
class SensitiveInputToken final {
public:
    SensitiveInputToken() noexcept = default;
    ~SensitiveInputToken() = default;
    SensitiveInputToken(const SensitiveInputToken &) = delete;
    auto operator=(const SensitiveInputToken &) -> SensitiveInputToken & = delete;
    SensitiveInputToken(SensitiveInputToken &&other) noexcept;
    auto operator=(SensitiveInputToken &&other) noexcept -> SensitiveInputToken &;

public:
    /// Get the unique diagnostic identifier, or zero for an invalid token.
    [[nodiscard]] auto id() const noexcept -> uint64_t { return _id; }
    /// Get the source location where the sensitivity request started.
    [[nodiscard]] auto sourceLocation() const noexcept -> std::source_location { return _sourceLocation; }
    /// Test if this token contains an identifier that can be submitted to `stopSensitiveInput()`.
    [[nodiscard]] auto isValid() const noexcept -> bool { return _id != 0U; }

private:
    friend auto startSensitiveInput(std::source_location location) -> SensitiveInputToken;
    friend void stopSensitiveInput(SensitiveInputToken token);

    SensitiveInputToken(uint64_t id, std::source_location sourceLocation) noexcept :
        _id{id}, _sourceLocation{sourceLocation} {}

private:
    uint64_t _id{};                         ///< Unique registry identifier.
    std::source_location _sourceLocation{}; ///< Start location for diagnostics.
};

/// Start secure buffering for the process-native standard input pipeline.
/// Requests may be stopped in any order. Redirected `stdIn()` targets are not affected.
/// @tested{StandardStreamsTest}
[[nodiscard]] auto startSensitiveInput(std::source_location location = std::source_location::current())
    -> SensitiveInputToken;

/// Stop one native standard-input sensitivity request.
/// @throws err::LogicError If the token is invalid or was already stopped.
/// @tested{StandardStreamsTest}
void stopSensitiveInput(SensitiveInputToken token);

/// A move-only scope that enables secure buffering for process-native standard input.
/// @tested{StandardStreamsTest}
class SensitiveInputScope final {
public:
    /// Start a sensitivity request at the caller's source location.
    explicit SensitiveInputScope(std::source_location location = std::source_location::current());
    ~SensitiveInputScope();
    SensitiveInputScope(const SensitiveInputScope &) = delete;
    auto operator=(const SensitiveInputScope &) -> SensitiveInputScope & = delete;
    SensitiveInputScope(SensitiveInputScope &&other) noexcept;
    auto operator=(SensitiveInputScope &&other) noexcept -> SensitiveInputScope &;

public:
    /// Test if this object owns an active request.
    [[nodiscard]] auto isActive() const noexcept -> bool { return _token.has_value(); }
    /// Stop the owned request now. Repeated calls have no effect.
    void reset() noexcept;

private:
    std::optional<SensitiveInputToken> _token; ///< The owned active request.
};

}
