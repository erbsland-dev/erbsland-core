// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "NetworkErrorReason_fwd.hpp"

#include "../../core/Definitions.hpp"

#include <cstdint>

namespace erbsland::network {

/// A machine-readable reason for an asynchronous network failure.
enum class NetworkErrorReason : std::uint8_t {
    Unknown,                       ///< The failure has no more specific portable reason.
    Timeout,                       ///< The operation exceeded its configured deadline.
    HostNotFound,                  ///< The resolver reported that the requested host does not exist.
    NoAddresses,                   ///< The resolver returned no supported IP addresses.
    HostResolutionFailed,          ///< The native host resolver failed for another reason.
    AddressInUse,                  ///< The requested local address or port is already in use.
    PermissionDenied,              ///< The platform denied the requested socket operation.
    NetworkUnreachable,            ///< No route to the requested network or host is available.
    ConnectionRefused,             ///< The remote endpoint refused the operation.
    ConnectionReset,               ///< The established connection was reset by its peer or the network.
    MessageTooLarge,               ///< A datagram exceeded a native transport limit.
    SocketOperationFailed,         ///< Another native socket operation failed.
    ResourceLimitExceeded,         ///< A configured finite resource quota is full.
    ConfigurationFailed,           ///< Required application connection configuration is unavailable.
    TlsProtocolFailure,            ///< The peer violated the TLS protocol.
    TlsAuthenticationFailure,      ///< Peer certificate or proof-of-possession authentication failed.
    TlsPolicyFailure,              ///< A TLS policy rejected the peer or negotiated parameters.
    TlsPeerAlert,                  ///< The peer sent a fatal TLS alert.
    TlsTruncation,                 ///< The transport ended without authenticated TLS close notification.
    TlsInternalFailure,            ///< A local TLS implementation operation failed.
    ContentSourceFailed,           ///< A static file or resource provider failed unexpectedly.
    ContentSinkFailed,             ///< A response output sink failed unexpectedly.
    HttpProtocolFailure,           ///< The peer violated HTTP syntax or framing.
    HttpResponseValidationFailure, ///< A response failed selected media, text, or JSON validation.
    HttpRedirectFailure,           ///< A redirect was rejected or could not satisfy a hard follow guard.
};

}
