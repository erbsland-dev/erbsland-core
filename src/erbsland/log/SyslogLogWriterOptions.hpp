// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "SyslogTransport.hpp"

#include "../network/HostEndpoint.hpp"
#include "../text/String.hpp"
#include "../unit/ByteLength.hpp"

#include <cstdint>

namespace erbsland::log {

/// Configuration for a syslog writer.
/// @tested{LogConfigurationParserTest}
class SyslogLogWriterOptions final {
public:
    constexpr static auto cNotSet = text::StringLiteral{"-"};
    constexpr static auto cDefaultApplicationName = text::StringLiteral{"erbsland-core"};
    constexpr static auto cDefaultTlsLabel = text::StringLiteral{"log/syslog"};

public:
    /// Create default UDP syslog settings.
    SyslogLogWriterOptions();

    /// Get the network transport.
    [[nodiscard]] auto transport() const noexcept -> SyslogTransport { return _transport; }
    /// Set the network transport.
    /// @param value The UDP, TCP, or TLS transport to use.
    /// @return These options for chained configuration.
    auto setTransport(SyslogTransport value) noexcept -> SyslogLogWriterOptions &;
    /// Get the target endpoint.
    [[nodiscard]] auto endpoint() const noexcept -> const network::HostEndpoint & { return _endpoint; }
    /// Set the target endpoint.
    /// @param value The remote address and port receiving syslog messages.
    /// @return These options for chained configuration.
    auto setEndpoint(network::HostEndpoint value) noexcept -> SyslogLogWriterOptions &;
    /// Get the RFC 5424 facility number.
    [[nodiscard]] auto facility() const noexcept -> uint8_t { return _facility; }
    /// Validate and set the RFC 5424 facility number.
    /// @param value The facility in the inclusive range zero through 23.
    /// @return These options for chained configuration.
    auto setFacility(uint8_t value) -> SyslogLogWriterOptions &;
    /// Get the RFC 5424 host name field.
    [[nodiscard]] auto hostName() const noexcept -> const text::String & { return _hostName; }
    /// Validate and set the RFC 5424 host name field.
    /// @param value The printable ASCII host name, or `-` for NILVALUE.
    /// @return These options for chained configuration.
    auto setHostName(text::String value) -> SyslogLogWriterOptions &;
    /// Get the RFC 5424 application name field.
    [[nodiscard]] auto applicationName() const noexcept -> const text::String & { return _applicationName; }
    /// Validate and set the RFC 5424 application name field.
    /// @param value The printable ASCII application name, or `-` for NILVALUE.
    /// @return These options for chained configuration.
    auto setApplicationName(text::String value) -> SyslogLogWriterOptions &;
    /// Get the RFC 5424 process identifier field.
    [[nodiscard]] auto processId() const noexcept -> const text::String & { return _processId; }
    /// Validate and set the RFC 5424 process identifier field.
    /// @param value The printable ASCII process identifier, or `-` for NILVALUE.
    /// @return These options for chained configuration.
    auto setProcessId(text::String value) -> SyslogLogWriterOptions &;
    /// Get the RFC 5424 message identifier field.
    [[nodiscard]] auto messageId() const noexcept -> const text::String & { return _messageId; }
    /// Validate and set the RFC 5424 message identifier field.
    /// @param value The printable ASCII message identifier, or `-` for NILVALUE.
    /// @return These options for chained configuration.
    auto setMessageId(text::String value) -> SyslogLogWriterOptions &;
    /// Get the TLS configuration label.
    [[nodiscard]] auto tlsConfigurationLabel() const noexcept -> const text::String & { return _tlsLabel; }
    /// Validate and set the TLS configuration label.
    /// @param value The nonempty network/TLS configuration label.
    /// @return These options for chained configuration.
    auto setTlsConfigurationLabel(text::String value) -> SyslogLogWriterOptions &;
    /// Get the maximum pending network-data size.
    [[nodiscard]] auto maximumPendingBytes() const noexcept -> unit::ByteLength { return _maximumPendingBytes; }
    /// Validate and set the maximum pending network-data size.
    /// @param value The positive byte limit for messages awaiting transmission.
    /// @return These options for chained configuration.
    auto setMaximumPendingBytes(unit::ByteLength value) -> SyslogLogWriterOptions &;

private:
    SyslogTransport _transport{SyslogTransport::Udp};       ///< Network transport for syslog delivery.
    network::HostEndpoint _endpoint;                        ///< Remote syslog endpoint.
    uint8_t _facility{1U};                                  ///< RFC 5424 facility number.
    text::String _hostName{cNotSet};                        ///< RFC 5424 HOSTNAME field.
    text::String _applicationName{cDefaultApplicationName}; ///< RFC 5424 APP-NAME field.
    text::String _processId{cNotSet};                       ///< RFC 5424 PROCID field.
    text::String _messageId{cNotSet};                       ///< RFC 5424 MSGID field.
    text::String _tlsLabel{cDefaultTlsLabel};               ///< Network/TLS configuration label.
    unit::ByteLength _maximumPendingBytes{1024U * 1024U};   ///< Pending network-data byte limit.
};

}
