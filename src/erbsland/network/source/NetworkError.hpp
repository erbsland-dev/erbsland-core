// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "NetworkErrorContext.hpp"

#include "../../err/RuntimeError.hpp"

namespace erbsland::network {

/// An asynchronous network operation error.
/// @tested{HostLookupTest NetworkFacadeTest}
class NetworkError final : public err::RuntimeError {
public:
    /// Create an asynchronous network error.
    /// @param context The structured operation context.
    /// @param cause The optional platform or lower-level cause.
    explicit NetworkError(NetworkErrorContext context, std::exception_ptr cause = {}) noexcept;

    // defaults
    ~NetworkError() override = default;

public: // overrides
    [[nodiscard]] auto diagnostic() const -> err::DiagnosticConstPtr override;

public: // accessors
    /// Get the structured network context.
    /// @return The retained error context.
    [[nodiscard]] auto context() const noexcept -> const NetworkErrorContext & { return _context; }

private:
    NetworkErrorContext _context; ///< The structured operation context.
};

}
