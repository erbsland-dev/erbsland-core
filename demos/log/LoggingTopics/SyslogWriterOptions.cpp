// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/log/all.hpp>

#include <memory>
#include <vector>

namespace demo {

/// The transport option chooses UDP datagrams, a TCP stream, or a TLS-protected TCP stream.
///
/// TCP and TLS use RFC 6587 octet-counted framing. Creating a writer validates the selection but does not open a
/// connection; network activity begins only after the configured manager delivers an entry.
void syslogTransport() {
    auto udpOptions = el::SyslogLogWriterOptions{};
    udpOptions.setTransport(el::SyslogTransport::Udp)
        .setEndpoint(el::HostEndpoint::fromStringOrThrow("192.0.2.10:514"_el));

    auto tcpOptions = el::SyslogLogWriterOptions{};
    tcpOptions.setTransport(el::SyslogTransport::Tcp)
        .setEndpoint(el::HostEndpoint::fromStringOrThrow("logs.example:601"_el));

    auto tlsOptions = el::SyslogLogWriterOptions{};
    tlsOptions.setTransport(el::SyslogTransport::Tls)
        .setEndpoint(el::HostEndpoint::fromStringOrThrow("logs.example:6514"_el));

    auto configuration = el::LogConfiguration{};
    configuration.addWriter(el::LogWriter::createForSyslog(udpOptions))
        .addWriter(el::LogWriter::createForSyslog(tcpOptions))
        .addWriter(el::LogWriter::createForSyslog(tlsOptions));

    el::io::printLine("UDP endpoint: "_el, udpOptions.endpoint().toString());
    el::io::printLine("TCP endpoint: "_el, tcpOptions.endpoint().toString());
    el::io::printLine("TLS endpoint: "_el, tlsOptions.endpoint().toString());
}

/// The endpoint option selects the collector host and nonzero transport port.
///
/// UDP needs a numeric IP address because the datagram writer does not resolve names. TCP and TLS accept either a
/// numeric address or a host name that the connection layer resolves.
void syslogEndpoint() {
    auto localOptions = el::SyslogLogWriterOptions{};

    auto remoteOptions = el::SyslogLogWriterOptions{};
    remoteOptions.setTransport(el::SyslogTransport::Tls)
        .setEndpoint(el::HostEndpoint::fromStringOrThrow("logs.example:6514"_el));

    auto configuration = el::LogConfiguration{};
    configuration.addWriter(el::LogWriter::createForSyslog(localOptions))
        .addWriter(el::LogWriter::createForSyslog(remoteOptions));

    el::io::printLine("Default endpoint: "_el, localOptions.endpoint().toString());
    el::io::printLine("Remote endpoint : "_el, remoteOptions.endpoint().toString());
}

/// The facility option places a message in the collector's RFC 5424 facility namespace.
///
/// The priority number is `facility * 8 + severity`. A warning has severity four, so facility one produces priority 12
/// and local-use facility 16 produces priority 132.
void syslogFacility() {
    auto userOptions = el::SyslogLogWriterOptions{};
    userOptions.setFacility(1U);
    auto localOptions = el::SyslogLogWriterOptions{};
    localOptions.setFacility(16U);

    el::io::printLine("Facility 1 : "_el, userOptions.facility());
    el::io::printLine("Facility 16: "_el, localOptions.facility());
}

/// The host-name option fills the RFC 5424 HOSTNAME field.
///
/// Use a stable machine identity understood by the collector, or keep the default `-` NILVALUE when no identity can be
/// stated reliably.
void syslogHostName() {
    auto options = el::SyslogLogWriterOptions{};
    options.setHostName("guild-hall"_el);
    el::io::printLine(options.hostName());
}

/// The application-name option fills the RFC 5424 APP-NAME field.
///
/// A short, stable service name helps a collector group messages across process restarts and hosts. The default is
/// `erbsland-core`.
void syslogApplicationName() {
    auto options = el::SyslogLogWriterOptions{};
    options.setApplicationName("explorer-guild"_el);
    el::io::printLine(options.applicationName());
}

/// The process-id option fills the RFC 5424 PROCID field.
///
/// Supply a process identifier when it helps distinguish concurrent instances, or retain the default `-` NILVALUE.
void syslogProcessId() {
    auto options = el::SyslogLogWriterOptions{};
    options.setProcessId("314"_el);
    el::io::printLine(options.processId());
}

/// The message-id option fills the RFC 5424 MSGID field.
///
/// Use a stable event family such as `route` or `startup`, rather than copying the changing message text into this
/// compact header field.
void syslogMessageId() {
    auto options = el::SyslogLogWriterOptions{};
    options.setMessageId("route"_el);
    el::io::printLine(options.messageId());
}

/// The TLS label selects the reusable network/TLS configuration used for a secure connection.
///
/// The label does not contain certificates or verification settings itself. It points the TLS connection to the
/// matching application-managed policy and defaults to `log/syslog`.
void syslogTlsLabel() {
    auto options = el::SyslogLogWriterOptions{};
    options.setTransport(el::SyslogTransport::Tls)
        .setEndpoint(el::HostEndpoint::fromStringOrThrow("logs.example:6514"_el))
        .setTlsConfigurationLabel("operations/syslog"_el);
    auto configuration = el::LogConfiguration{};
    configuration.addWriter(el::LogWriter::createForSyslog(options));

    el::io::printLine("TLS configuration label: "_el, options.tlsConfigurationLabel());
}

/// The pending-byte limit bounds encoded syslog data waiting for transport acceptance.
///
/// A message that cannot fit is dropped before network activity starts, preventing an unavailable collector from
/// causing unbounded memory growth in the application.
void syslogMaximumPendingBytes() {
    auto options = el::SyslogLogWriterOptions{};
    options.setMaximumPendingBytes(el::ByteLength{32U});
    auto configuration = el::LogConfiguration{};
    configuration.addWriter(el::LogWriter::createForSyslog(options));
    el::io::printLine("Maximum pending bytes: "_el, options.maximumPendingBytes());
}

}
