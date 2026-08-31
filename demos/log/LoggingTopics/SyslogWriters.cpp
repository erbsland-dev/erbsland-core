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
/// A route can be assembled without opening a connection; this demo formats a fixed message without network delivery.
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

    // Construct the writer route used by an application configuration.
    const auto writer = std::make_shared<el::SyslogLogWriter>(options);
    auto configuration = el::LogConfiguration{};
    configuration.addWriter(writer, el::LogWriterFilter{el::LogLevels{el::LogLevel::Warning, el::LogLevel::Error}});

    // Preview a deterministic RFC 5424 message without contacting the configured endpoint.
    const auto entry = el::LogEntry{
        1U,
        el::DateTime{el::Date::fromYearMonthDay(2026, 9, 1), el::Time{el::Hour{7}, el::Minute{45}}},
        el::LogLevel::Warning,
        el::LogPath{"guild/route"_el},
        "Pass closed."_el};
    const auto line = el::LogLine{std::vector<el::LogLineSegment>{{el::LogLinePart::Message, "Pass closed."_el}}};
    const auto message = el::SyslogLogWriter::formatMessage(entry, line, options);

    el::io::printLine(message);
    el::io::printLine("TCP/TLS frame: "_el, el::SyslogLogWriter::frameMessage(message));
}

}
