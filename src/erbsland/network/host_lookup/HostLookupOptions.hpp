// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "HostLookupOptions_fwd.hpp"

#include "../../time/TimeDelta.hpp"
#include "../../unit/ItemCount.hpp"

namespace erbsland::network {

/// Options for one asynchronous host lookup operation.
/// @tested{HostLookupTest}
class HostLookupOptions final {
public:
    /// The default deadline for the complete lookup operation.
    inline static const auto cDefaultTimeout = time::TimeDelta::seconds(10);
    /// The default maximum number of native resolver attempts.
    inline static const auto cDefaultMaximumAttempts = unit::ItemCount{2U};
    /// The default delay between native resolver attempts.
    inline static const auto cDefaultRetryDelay = time::TimeDelta::milliseconds(100);

public: // accessors
    /// Get the deadline for the complete lookup operation.
    [[nodiscard]] auto timeout() const noexcept -> time::TimeDelta { return _timeout; }
    /// Set the deadline for the complete lookup operation.
    auto setTimeout(const time::TimeDelta value) noexcept -> HostLookupOptions & {
        _timeout = value;
        return *this;
    }
    /// Get the maximum number of native resolver attempts.
    [[nodiscard]] auto maximumAttempts() const noexcept -> unit::ItemCount { return _maximumAttempts; }
    /// Set the maximum number of native resolver attempts.
    auto setMaximumAttempts(const unit::ItemCount value) noexcept -> HostLookupOptions & {
        _maximumAttempts = value;
        return *this;
    }
    /// Get the delay between native resolver attempts.
    [[nodiscard]] auto retryDelay() const noexcept -> time::TimeDelta { return _retryDelay; }
    /// Set the delay between native resolver attempts.
    auto setRetryDelay(const time::TimeDelta value) noexcept -> HostLookupOptions & {
        _retryDelay = value;
        return *this;
    }

private:
    time::TimeDelta _timeout{cDefaultTimeout};                 ///< Complete-operation deadline.
    unit::ItemCount _maximumAttempts{cDefaultMaximumAttempts}; ///< Maximum native resolver attempts.
    time::TimeDelta _retryDelay{cDefaultRetryDelay};           ///< Delay between resolver attempts.
};

}
