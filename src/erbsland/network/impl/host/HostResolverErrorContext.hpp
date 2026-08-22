// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../system/PlatformErrorContext.hpp"

namespace erbsland::network::impl {

/// Immutable diagnostic details from a native host resolver.
/// @tested{HostLookupTest HostResolverTest}
class HostResolverErrorContext final : public system::PlatformErrorContext {
public:
    /// Create resolver error details.
    /// @param errorCode The native resolver error code.
    /// @param errorMessage The native resolver message.
    /// @param category The portable error category.
    /// @param isRetryable Whether retrying the resolver may succeed.
    HostResolverErrorContext(
        int errorCode,
        text::String errorMessage,
        system::PlatformErrorCategory category,
        bool isRetryable = false) noexcept;

public: // implement PlatformErrorContext
    [[nodiscard]] auto category() const noexcept -> system::PlatformErrorCategory override;
    [[nodiscard]] auto toString() const noexcept -> text::String override;
    [[nodiscard]] auto toTextDocument() const -> text::TextDocument override;

public: // accessors
    /// Get the native resolver error code.
    [[nodiscard]] auto errorCode() const noexcept -> int { return _errorCode; }
    /// Get the native resolver error message.
    [[nodiscard]] auto errorMessage() const noexcept -> const text::String & { return _errorMessage; }
    /// Test if the resolver failure is portable and safe to retry.
    [[nodiscard]] auto isRetryable() const noexcept -> bool { return _isRetryable; }

private:
    int _errorCode{};                        ///< Native resolver error code.
    text::String _errorMessage;              ///< Native resolver error message.
    system::PlatformErrorCategory _category; ///< Portable error category.
    bool _isRetryable{false};                ///< Whether retrying may succeed.
};

}
