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

#include "../../conf/vr/all.hpp"
#include "../../conf/vr/builder/all.hpp"
#include "../../text/Literals.hpp"
#include "../../text/StringList.hpp"

namespace erbsland::log::impl {

using namespace text;
using namespace text::literals;

auto createLogConfigurationRules() -> conf::vr::RulesPtr {
    using namespace conf::vr::builder;
    using conf::vr::RuleType;
    static const auto cWriterTypes = StringList{"console"_el, "file"_el, "syslog"_el};
    constexpr auto cFormatName = "format"_el;
    auto rb = conf::vr::RulesBuilder{};

    rb.addRule(cFormatName, RuleType::Section);
    auto format = rb.createBranch(cFormatName);
    addLogLineFormatRules(format);

    rb.addRule(
        "queue"_el,
        RuleType::Section,
        Title("Queue Limits"_el),
        Description("Controls bounded queue capacity, message size, and graceful shutdown."_el),
        IsOptional());
    auto queue = rb.createBranch("queue"_el);
    queue.addRule(
        "maximum_entries"_el,
        RuleType::Integer,
        Title("Maximum Entries"_el),
        Description("The maximum number of entries retained in the manager queue."_el),
        Minimum(1),
        IsOptional());
    queue.addRule(
        "maximum_bytes"_el,
        RuleType::Integer,
        Title("Maximum Queue Bytes"_el),
        Description("The maximum number of sanitized message bytes retained in the manager queue."_el),
        Minimum(1),
        IsOptional());
    queue.addRule(
        "reserved_error_entries"_el,
        RuleType::Integer,
        Title("Reserved Error Entries"_el),
        Description("The queue entry slots reserved for warning and error messages."_el),
        Minimum(0),
        IsOptional());
    queue.addRule(
        "reserved_error_bytes"_el,
        RuleType::Integer,
        Title("Reserved Error Bytes"_el),
        Description("The queue message bytes reserved for warning and error messages."_el),
        Minimum(0),
        IsOptional());
    queue.addRule(
        "maximum_message_bytes"_el,
        RuleType::Integer,
        Title("Maximum Message Bytes"_el),
        Description("The producer-side byte limit for one sanitized log message."_el),
        Minimum(1),
        IsOptional());
    queue.addRule(
        "shutdown_timeout_ms"_el,
        RuleType::Integer,
        Title("Shutdown Timeout"_el),
        Description("The graceful manager shutdown deadline in milliseconds."_el),
        Minimum(0),
        IsOptional());

    rb.addRule(
        "trace_sections"_el,
        RuleType::ValueList,
        Title("Trace Sections"_el),
        Description("The trace section identifiers enabled by this configuration."_el),
        IsOptional(),
        EntryType{RuleType::Text},
        EntryTitle("Trace Section"_el),
        EntryDescription("One trace section identifier to enable."_el));

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
    auto writer = rb.createBranch("writers.vr_entry"_el);
    writer.addRule(cFormatName, RuleType::Section);
    auto writerFormat = writer.createBranch(cFormatName);
    addLogLineFormatRules(writerFormat);
    writer.addRule(
        "type"_el,
        RuleType::Text,
        Title("Writer Type"_el),
        Description("The built-in writer type: console, file, or syslog."_el),
        In(cWriterTypes));
    writer.addRule(
        "levels"_el,
        RuleType::ValueList,
        Title("Accepted Levels"_el),
        Description("The levels routed to this writer. If omitted, all levels are accepted."_el),
        IsOptional(),
        EntryType{RuleType::Text},
        EntryTitle("Log Level"_el),
        EntryDescription("One accepted level: trace, information, info, warning, warn, error, or all."_el),
        EntryConstraints{In(LogLevel::allStrings())});
    writer.addRule(
        "paths"_el,
        RuleType::ValueList,
        Title("Accepted Paths"_el),
        Description("The log path roots routed to this writer. If omitted, every path is accepted."_el),
        IsOptional(),
        EntryType{RuleType::Text},
        EntryTitle("Log Path"_el),
        EntryDescription("One lowercase slash-delimited log path root matched by complete path segments."_el));
    writer.addRule(
        "path"_el,
        RuleType::Text,
        Title("File Path"_el),
        Description("The active log file path used by a file writer."_el),
        IsOptional());
    writer.addRule(
        "mode"_el,
        RuleType::Text,
        Title("File Open Mode"_el),
        Description("The initial file open mode: append or overwrite."_el),
        In(LogFileMode::allStrings()),
        IsOptional());
    writer.addRule(
        "rotation"_el,
        RuleType::Text,
        Title("File Rotation"_el),
        Description("The file rotation mode: none, hourly, daily, weekly, or size."_el),
        In(LogFileRotation::allStrings()),
        IsOptional());
    writer.addRule(
        "maximum_size"_el,
        RuleType::Integer,
        Title("Maximum File Size"_el),
        Description("The active-file byte threshold used by size rotation."_el),
        Minimum(1),
        IsOptional());
    writer.addRule(
        "retention"_el,
        RuleType::Integer,
        Title("Archive Retention"_el),
        Description("The maximum number of rotated file archives to retain."_el),
        Minimum(0),
        IsOptional());
    writer.addRule(
        "endpoint"_el,
        RuleType::Text,
        Title("Syslog Endpoint"_el),
        Description("The remote host and port receiving syslog messages."_el),
        IsOptional());
    writer.addRule(
        "transport"_el,
        RuleType::Text,
        Title("Syslog Transport"_el),
        Description("The syslog transport: udp, tcp, or tls."_el),
        In(SyslogTransport::allStrings()),
        IsOptional());
    writer.addRule(
        "facility"_el,
        RuleType::Integer,
        Title("Syslog Facility"_el),
        Description("The RFC 5424 facility number from zero through 23."_el),
        Minimum(0),
        Maximum(23),
        IsOptional());
    writer.addRule(
        "host_name"_el,
        RuleType::Text,
        Title("Syslog Host Name"_el),
        Description("The RFC 5424 HOSTNAME field, or - for NILVALUE."_el),
        IsOptional());
    writer.addRule(
        "application_name"_el,
        RuleType::Text,
        Title("Syslog Application Name"_el),
        Description("The RFC 5424 APP-NAME field, or - for NILVALUE."_el),
        IsOptional());
    writer.addRule(
        "process_id"_el,
        RuleType::Text,
        Title("Syslog Process ID"_el),
        Description("The RFC 5424 PROCID field, or - for NILVALUE."_el),
        IsOptional());
    writer.addRule(
        "message_id"_el,
        RuleType::Text,
        Title("Syslog Message ID"_el),
        Description("The RFC 5424 MSGID field, or - for NILVALUE."_el),
        IsOptional());
    writer.addRule(
        "tls_label"_el,
        RuleType::Text,
        Title("TLS Configuration Label"_el),
        Description("The network/TLS configuration label used by a TLS syslog writer."_el),
        IsOptional());
    writer.addRule(
        "maximum_pending_bytes"_el,
        RuleType::Integer,
        Title("Maximum Pending Bytes"_el),
        Description("The maximum encoded syslog data retained while awaiting network delivery."_el),
        Minimum(1),
        IsOptional());
    writer.addRule(
        "line_indent"_el,
        RuleType::Integer,
        Title("Line Indent"_el),
        Description("The leading columns applied to every console output line."_el),
        Minimum(0),
        IsOptional());
    writer.addRule(
        "first_line_indent"_el,
        RuleType::Integer,
        Title("First-Line Indent"_el),
        Description("The first console line indent, or -1 to reuse line_indent."_el),
        Minimum(-1),
        IsOptional());
    writer.addRule(
        "wrapped_line_indent"_el,
        RuleType::Integer,
        Title("Wrapped-Line Indent"_el),
        Description("The wrapped console line indent, or -1 to reuse line_indent."_el),
        Minimum(-1),
        IsOptional());
    writer.addRule(
        "maximum_line_wraps"_el,
        RuleType::Integer,
        Title("Maximum Line Wraps"_el),
        Description("The maximum automatic wraps per source line, or zero for unlimited wrapping."_el),
        Minimum(0),
        IsOptional());

    return rb.takeRules();
}

void addLogLineFormatRules(conf::vr::RulesBuilder &builder) {
    using namespace conf::vr::builder;
    using conf::vr::RuleType;
    builder.configureRoot(
        Title("Line Format"_el),
        Description("Controls how log entries are rendered before they are passed to writers."_el),
        IsOptional());
    builder.addRule(
        "pattern"_el,
        RuleType::Text,
        Title("Line Pattern"_el),
        Description(
            "The line pattern using {time}, {level}, {name}, and {message}; doubled braces produce literal "_el
            "braces."_el),
        IsOptional());
    builder.addRule(
        "timestamp_zone"_el,
        RuleType::Text,
        Title("Timestamp Zone"_el),
        Description("The timestamp rendering zone: utc or local. Stored timestamps always remain UTC."_el),
        In(LogTimestampZone::allStrings()),
        IsOptional());
    builder.addRule(
        "level"_el,
        RuleType::Text,
        Title("Level Format"_el),
        Description("The rendered level format: short_upper, short_lower, full_lower, or full_upper."_el),
        In(LogLevelFormat::allStrings()),
        IsOptional());
    builder.addRule(
        "name"_el,
        RuleType::Text,
        Title("Name Format"_el),
        Description("The rendered stream-name format: full, leaf, head_and_leaf, or left_truncated."_el),
        In(LogNameFormat::allStrings()),
        IsOptional());
    builder.addRule(
        "name_limit"_el,
        RuleType::Integer,
        Title("Name Limit"_el),
        Description("The maximum stream-name length in code points for the left_truncated format."_el),
        Minimum(0),
        IsOptional());
    builder.addRule(
        "message_truncation"_el,
        RuleType::Text,
        Title("Message Truncation"_el),
        Description("The message truncation mode: none, first_line, characters, or total_line."_el),
        In(LogMessageTruncation::allStrings()),
        IsOptional());
    builder.addRule(
        "message_limit"_el,
        RuleType::Integer,
        Title("Message Limit"_el),
        Description("The code-point limit used by the selected message truncation mode."_el),
        Minimum(0),
        IsOptional());
    builder.addRule(
        "truncation_mark"_el,
        RuleType::Text,
        Title("Truncation Mark"_el),
        Description("The marker appended to text truncated during line rendering."_el),
        IsOptional());
}

}
