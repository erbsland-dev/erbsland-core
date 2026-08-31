// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "LogConfigurationRules.hpp"

#include "../LogFileMode.hpp"
#include "../LogFileRotation.hpp"
#include "../LogLevel.hpp"
#include "../LogLevelFormat.hpp"
#include "../LogMessageTruncation.hpp"
#include "../LogNameFormat.hpp"
#include "../LogTimestampZone.hpp"
#include "../SyslogTransport.hpp"

#include "../../conf/vr/builder/all.hpp"
#include "../../conf/vr/RulesBuilder.hpp"
#include "../../text/Literals.hpp"
#include "../../text/StringList.hpp"

namespace erbsland::log::impl {

using namespace text::literals;

auto createLogConfigurationRules() -> conf::vr::RulesPtr {
    using namespace conf::vr::builder;
    using conf::vr::RuleType;
    static const auto cWriterTypes = text::StringList{"console"_el, "file"_el, "last_errors"_el, "syslog"_el};
    auto rb = conf::vr::RulesBuilder{};

    rb.addRule(
        "format"_el,
        RuleType::Section,
        Title("Line Format"_el),
        Description("Controls how log entries are rendered before they are passed to writers."_el),
        IsOptional());
    rb.addRule(
        "format.pattern"_el,
        RuleType::Text,
        Title("Line Pattern"_el),
        Description(
            "The line pattern using {time}, {level}, {name}, and {message}; doubled braces produce literal "_el
            "braces."_el),
        IsOptional());
    rb.addRule(
        "format.timestamp_zone"_el,
        RuleType::Text,
        Title("Timestamp Zone"_el),
        Description("The timestamp rendering zone: utc or local. Stored timestamps always remain UTC."_el),
        In(LogTimestampZone::allStrings()),
        IsOptional());
    rb.addRule(
        "format.level"_el,
        RuleType::Text,
        Title("Level Format"_el),
        Description("The rendered level format: short_upper, short_lower, full_lower, or full_upper."_el),
        In(LogLevelFormat::allStrings()),
        IsOptional());
    rb.addRule(
        "format.name"_el,
        RuleType::Text,
        Title("Name Format"_el),
        Description("The rendered stream-name format: full, leaf, head_and_leaf, or left_truncated."_el),
        In(LogNameFormat::allStrings()),
        IsOptional());
    rb.addRule(
        "format.name_limit"_el,
        RuleType::Integer,
        Title("Name Limit"_el),
        Description("The maximum stream-name length in code points for the left_truncated format."_el),
        Minimum(0),
        IsOptional());
    rb.addRule(
        "format.message_truncation"_el,
        RuleType::Text,
        Title("Message Truncation"_el),
        Description("The message truncation mode: none, first_line, characters, or total_line."_el),
        In(LogMessageTruncation::allStrings()),
        IsOptional());
    rb.addRule(
        "format.message_limit"_el,
        RuleType::Integer,
        Title("Message Limit"_el),
        Description("The code-point limit used by the selected message truncation mode."_el),
        Minimum(0),
        IsOptional());
    rb.addRule(
        "format.truncation_mark"_el,
        RuleType::Text,
        Title("Truncation Mark"_el),
        Description("The marker appended to text truncated during line rendering."_el),
        IsOptional());

    rb.addRule(
        "queue"_el,
        RuleType::Section,
        Title("Queue Limits"_el),
        Description("Controls bounded queue capacity, message size, and graceful shutdown."_el),
        IsOptional());
    rb.addRule(
        "queue.maximum_entries"_el,
        RuleType::Integer,
        Title("Maximum Entries"_el),
        Description("The maximum number of entries retained in the manager queue."_el),
        Minimum(1),
        IsOptional());
    rb.addRule(
        "queue.maximum_bytes"_el,
        RuleType::Integer,
        Title("Maximum Queue Bytes"_el),
        Description("The maximum number of sanitized message bytes retained in the manager queue."_el),
        Minimum(1),
        IsOptional());
    rb.addRule(
        "queue.reserved_error_entries"_el,
        RuleType::Integer,
        Title("Reserved Error Entries"_el),
        Description("The queue entry slots reserved for warning and error messages."_el),
        Minimum(0),
        IsOptional());
    rb.addRule(
        "queue.reserved_error_bytes"_el,
        RuleType::Integer,
        Title("Reserved Error Bytes"_el),
        Description("The queue message bytes reserved for warning and error messages."_el),
        Minimum(0),
        IsOptional());
    rb.addRule(
        "queue.maximum_message_bytes"_el,
        RuleType::Integer,
        Title("Maximum Message Bytes"_el),
        Description("The producer-side byte limit for one sanitized log message."_el),
        Minimum(1),
        IsOptional());
    rb.addRule(
        "queue.shutdown_timeout_ms"_el,
        RuleType::Integer,
        Title("Shutdown Timeout"_el),
        Description("The graceful manager shutdown deadline in milliseconds."_el),
        Minimum(0),
        IsOptional());

    rb.addRule(
        "trace_sections"_el,
        RuleType::ValueList,
        Title("Trace Sections"_el),
        Description("The case-sensitive trace section identifiers enabled by this configuration."_el),
        IsOptional());
    rb.addRule(
        "trace_sections.vr_entry"_el,
        RuleType::Text,
        Title("Trace Section"_el),
        Description("One case-sensitive trace section identifier to enable."_el));

    rb.addRule(
        "writers"_el,
        RuleType::SectionList,
        Title("Log Writers"_el),
        Description("The ordered built-in writer definitions and their route filters."_el),
        IsOptional());
    rb.addRule(
        "writers.vr_entry"_el,
        RuleType::Section,
        Title("Log Writer"_el),
        Description("One built-in writer and the levels and paths routed to it."_el));
    rb.addRule(
        "writers.vr_entry.type"_el,
        RuleType::Text,
        Title("Writer Type"_el),
        Description("The built-in writer type: console, file, last_errors, or syslog."_el),
        In(cWriterTypes));
    rb.addRule(
        "writers.vr_entry.levels"_el,
        RuleType::ValueList,
        Title("Accepted Levels"_el),
        Description("The levels routed to this writer. If omitted, all levels are accepted."_el),
        IsOptional());
    rb.addRule(
        "writers.vr_entry.levels.vr_entry"_el,
        RuleType::Text,
        Title("Log Level"_el),
        Description("One accepted level: trace, information, info, warning, warn, error, or all."_el),
        In(LogLevel::allStrings()));
    rb.addRule(
        "writers.vr_entry.paths"_el,
        RuleType::ValueList,
        Title("Accepted Paths"_el),
        Description("The log path roots routed to this writer. If omitted, every path is accepted."_el),
        IsOptional());
    rb.addRule(
        "writers.vr_entry.paths.vr_entry"_el,
        RuleType::Text,
        Title("Log Path"_el),
        Description("One lowercase slash-delimited log path root matched by complete path segments."_el));
    rb.addRule(
        "writers.vr_entry.path"_el,
        RuleType::Text,
        Title("File Path"_el),
        Description("The active log file path used by a file writer."_el),
        IsOptional());
    rb.addRule(
        "writers.vr_entry.mode"_el,
        RuleType::Text,
        Title("File Open Mode"_el),
        Description("The initial file open mode: append or overwrite."_el),
        In(LogFileMode::allStrings()),
        IsOptional());
    rb.addRule(
        "writers.vr_entry.rotation"_el,
        RuleType::Text,
        Title("File Rotation"_el),
        Description("The file rotation mode: none, hourly, daily, weekly, or size."_el),
        In(LogFileRotation::allStrings()),
        IsOptional());
    rb.addRule(
        "writers.vr_entry.maximum_size"_el,
        RuleType::Integer,
        Title("Maximum File Size"_el),
        Description("The active-file byte threshold used by size rotation."_el),
        Minimum(1),
        IsOptional());
    rb.addRule(
        "writers.vr_entry.retention"_el,
        RuleType::Integer,
        Title("Archive Retention"_el),
        Description("The maximum number of rotated file archives to retain."_el),
        Minimum(0),
        IsOptional());
    rb.addRule(
        "writers.vr_entry.capacity"_el,
        RuleType::Integer,
        Title("Last-Error Capacity"_el),
        Description("The maximum number of entries retained by a last_errors writer."_el),
        Minimum(1),
        IsOptional());
    rb.addRule(
        "writers.vr_entry.endpoint"_el,
        RuleType::Text,
        Title("Syslog Endpoint"_el),
        Description("The remote host and port receiving syslog messages."_el),
        IsOptional());
    rb.addRule(
        "writers.vr_entry.transport"_el,
        RuleType::Text,
        Title("Syslog Transport"_el),
        Description("The syslog transport: udp, tcp, or tls."_el),
        In(SyslogTransport::allStrings()),
        IsOptional());
    rb.addRule(
        "writers.vr_entry.facility"_el,
        RuleType::Integer,
        Title("Syslog Facility"_el),
        Description("The RFC 5424 facility number from zero through 23."_el),
        Minimum(0),
        Maximum(23),
        IsOptional());
    rb.addRule(
        "writers.vr_entry.host_name"_el,
        RuleType::Text,
        Title("Syslog Host Name"_el),
        Description("The RFC 5424 HOSTNAME field, or - for NILVALUE."_el),
        IsOptional());
    rb.addRule(
        "writers.vr_entry.application_name"_el,
        RuleType::Text,
        Title("Syslog Application Name"_el),
        Description("The RFC 5424 APP-NAME field, or - for NILVALUE."_el),
        IsOptional());
    rb.addRule(
        "writers.vr_entry.process_id"_el,
        RuleType::Text,
        Title("Syslog Process ID"_el),
        Description("The RFC 5424 PROCID field, or - for NILVALUE."_el),
        IsOptional());
    rb.addRule(
        "writers.vr_entry.message_id"_el,
        RuleType::Text,
        Title("Syslog Message ID"_el),
        Description("The RFC 5424 MSGID field, or - for NILVALUE."_el),
        IsOptional());
    rb.addRule(
        "writers.vr_entry.tls_label"_el,
        RuleType::Text,
        Title("TLS Configuration Label"_el),
        Description("The network/TLS configuration label used by a TLS syslog writer."_el),
        IsOptional());
    rb.addRule(
        "writers.vr_entry.maximum_pending_bytes"_el,
        RuleType::Integer,
        Title("Maximum Pending Bytes"_el),
        Description("The maximum encoded syslog data retained while awaiting network delivery."_el),
        Minimum(1),
        IsOptional());
    rb.addRule(
        "writers.vr_entry.line_indent"_el,
        RuleType::Integer,
        Title("Line Indent"_el),
        Description("The leading columns applied to every console output line."_el),
        Minimum(0),
        IsOptional());
    rb.addRule(
        "writers.vr_entry.first_line_indent"_el,
        RuleType::Integer,
        Title("First-Line Indent"_el),
        Description("The first console line indent, or -1 to reuse line_indent."_el),
        Minimum(-1),
        IsOptional());
    rb.addRule(
        "writers.vr_entry.wrapped_line_indent"_el,
        RuleType::Integer,
        Title("Wrapped-Line Indent"_el),
        Description("The wrapped console line indent, or -1 to reuse line_indent."_el),
        Minimum(-1),
        IsOptional());
    rb.addRule(
        "writers.vr_entry.maximum_line_wraps"_el,
        RuleType::Integer,
        Title("Maximum Line Wraps"_el),
        Description("The maximum automatic wraps per source line, or zero for unlimited wrapping."_el),
        Minimum(0),
        IsOptional());

    return rb.takeRules();
}

}
