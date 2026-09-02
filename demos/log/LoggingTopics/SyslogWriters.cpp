// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/log/all.hpp>

#include <cstdint>
#include <memory>
#include <vector>

namespace demo {

/// A syslog writer sends RFC 5424 messages over UDP, TCP, or TLS.
///
/// The options describe the endpoint, facility, RFC header fields, TLS configuration label, and bounded pending data.
/// A route can be assembled without opening a connection.
void syslogWriters() {
    auto options = el::SyslogLogWriterOptions{};
    options.setTransport(el::SyslogTransport::Tls)
        .setEndpoint(el::HostEndpoint::fromStringOrThrow("logs.example:6514"_el))
        .setFacility(1U)
        .setHostName("guild-hall"_el)
        .setApplicationName("explorer-guild"_el)
        .setProcessId("314"_el)
        .setMessageId("route"_el)
        .setTlsConfigurationLabel("guild/syslog"_el)
        .setMaximumPendingBytes(el::ByteLength{256U * 1024U});

    auto configuration = el::LogConfiguration{};
    configuration.addWriter(
        el::LogWriter::createForSyslog(options),
        el::LogWriterFilter{el::LogLevels{el::LogLevel::Warning, el::LogLevel::Error}});

    el::io::printLine("Syslog endpoint: "_el, options.endpoint().toString());
}

}
