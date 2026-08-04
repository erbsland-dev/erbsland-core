// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "SensitiveInput.hpp"

#include <optional>
#include <source_location>

namespace erbsland::stream::io {

/// A move-only scope that enables secure buffering for process-native standard input.
/// @tested{StandardStreamsTest}
class SensitiveInputScope final {
public:
    /// Start a sensitivity request at the caller's source location.
    explicit SensitiveInputScope(std::source_location location = std::source_location::current());
    /// Stop the owned sensitivity request.
    ~SensitiveInputScope();

    // defaults/deletions
    SensitiveInputScope(const SensitiveInputScope &) = delete;
    auto operator=(const SensitiveInputScope &) -> SensitiveInputScope & = delete;
    SensitiveInputScope(SensitiveInputScope &&other) noexcept;
    /// Move another sensitive-input scope into this scope.
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
