// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Http1FailureReason.hpp"
#include "Http1TransactionFailure_fwd.hpp"

#include "../../../../text/String.hpp"
#include "../../../source/NetworkErrorContext.hpp"

#include <cstdint>
#include <optional>
#include <utility>

namespace erbsland::network::impl {

/// A categorized internal failure from one HTTP/1 transaction.
/// @tested{Http1TransactionTest}
class Http1TransactionFailure final {
public:
    /// The broad failure category.
    enum class Kind : std::uint8_t {
        Protocol,          ///< The peer violated HTTP/1 framing or syntax.
        ResourceLimit,     ///< A configured HTTP transaction bound was exceeded.
        Timeout,           ///< One transaction deadline expired.
        Transport,         ///< The underlying connection failed.
        Callback,          ///< An application callback threw an exception.
        UnsupportedSwitch, ///< The peer switched to an unsupported opaque protocol.
    };
    /// The transaction phase that failed.
    enum class Phase : std::uint8_t {
        Headers,   ///< Incoming start line or main fields.
        Body,      ///< Incoming body or trailers.
        Total,     ///< Absolute transaction lifetime.
        Closing,   ///< Graceful connection closure.
        Transport, ///< Underlying connection processing.
    };

public:
    /// Create a local HTTP transaction failure.
    Http1TransactionFailure(
        Kind kind,
        Phase phase,
        text::String description,
        Http1FailureReason protocolReason = Http1FailureReason::None) :
        _kind{kind}, _phase{phase}, _description{std::move(description)}, _protocolReason{protocolReason} {}
    /// Create an underlying transport failure.
    explicit Http1TransactionFailure(NetworkErrorContext context) :
        _kind{Kind::Transport},
        _phase{Phase::Transport},
        _description{context.description()},
        _transportContext{std::move(context)} {}

public:
    /// Get the broad failure kind.
    [[nodiscard]] auto kind() const noexcept -> Kind { return _kind; }
    /// Get the transaction phase.
    [[nodiscard]] auto phase() const noexcept -> Phase { return _phase; }
    /// Get the codec category, if applicable.
    [[nodiscard]] auto protocolReason() const noexcept -> Http1FailureReason { return _protocolReason; }
    /// Get the nonsensitive local description.
    [[nodiscard]] auto description() const noexcept -> const text::String & { return _description; }
    /// Get underlying transport details, if applicable.
    [[nodiscard]] auto transportContext() const noexcept -> const std::optional<NetworkErrorContext> & {
        return _transportContext;
    }

private:
    Kind _kind;                                                   ///< Broad failure kind.
    Phase _phase;                                                 ///< Transaction phase.
    text::String _description;                                    ///< Nonsensitive diagnostic.
    Http1FailureReason _protocolReason{Http1FailureReason::None}; ///< Optional codec category.
    std::optional<NetworkErrorContext> _transportContext;         ///< Optional underlying context.
};

}
