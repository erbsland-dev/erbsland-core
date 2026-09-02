// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "LogConfigurationParser.hpp"

#include "ConsoleLogWriterOptions.hpp"
#include "FileLogWriterOptions.hpp"
#include "LogWriter.hpp"
#include "SyslogLogWriterOptions.hpp"
#include "SyslogTransport.hpp"

#include "impl/LogConfigurationRules.hpp"

#include "../conf/Value.hpp"
#include "../conf/vr/Rules.hpp"
#include "../err/ParameterError.hpp"
#include "../network/HostEndpoint.hpp"
#include "../path/Path.hpp"
#include "../text/CaseSensitivity.hpp"
#include "../text/Literals.hpp"

#include <algorithm>
#include <memory>
#include <utility>

namespace erbsland::log {

using namespace text::literals;

auto LogConfigurationParser::validationRules() -> const conf::vr::RulesPtr & {
    static const auto cRules = impl::createLogConfigurationRules();
    return cRules;
}

auto LogConfigurationParser::version() -> conf::Integer {
    return conf::Integer{1};
}

auto LogConfigurationParser::parse(const conf::ValuePtr &sectionValue) const -> LogConfiguration {
    if (sectionValue == nullptr) {
        throw err::ParameterError{"A log configuration branch must not be empty."_el, "branch"_el};
    }
    validationRules()->validate(sectionValue, version());
    auto result = LogConfiguration{};
    auto format = LogLineFormat{};
    if (const auto value = sectionValue->value("format"_el); value != nullptr) {
        if (value->hasValue("pattern"_el)) {
            format.setPattern(value->getTextOrThrow("pattern"_el));
        }
        if (value->hasValue("timestamp_zone"_el)) {
            format.setTimestampZone(LogTimestampZone::fromStringOrThrow(value->getTextOrThrow("timestamp_zone"_el)));
        }
        if (value->hasValue("level"_el)) {
            format.setLevelFormat(LogLevelFormat::fromStringOrThrow(value->getTextOrThrow("level"_el)));
        }
        if (value->hasValue("name"_el)) {
            format.setNameFormat(LogNameFormat::fromStringOrThrow(value->getTextOrThrow("name"_el)));
        }
        if (value->hasValue("name_limit"_el)) {
            format.setNameLimit(unit::CpLength::fromSizeT(value->getOrThrow<std::size_t>("name_limit"_el)));
        }
        if (value->hasValue("message_truncation"_el)) {
            format.setMessageTruncation(
                LogMessageTruncation::fromStringOrThrow(value->getTextOrThrow("message_truncation"_el)));
        }
        if (value->hasValue("message_limit"_el)) {
            format.setMessageLimit(unit::CpLength::fromSizeT(value->getOrThrow<std::size_t>("message_limit"_el)));
        }
        if (value->hasValue("truncation_mark"_el)) {
            format.setTruncationMark(value->getTextOrThrow("truncation_mark"_el));
        }
    }
    result.setLineFormat(std::move(format));

    auto managerOptions = LogManagerOptions{};
    if (const auto value = sectionValue->value("queue"_el); value != nullptr) {
        if (value->hasValue("maximum_entries"_el)) {
            managerOptions.setMaximumEntries(value->getOrThrow<std::size_t>("maximum_entries"_el));
        }
        if (value->hasValue("maximum_bytes"_el)) {
            managerOptions.setMaximumBytes(
                unit::ByteLength::fromSizeT(value->getOrThrow<std::size_t>("maximum_bytes"_el)));
        }
        managerOptions.setReservedErrorEntries(
            value->hasValue("reserved_error_entries"_el)
                ? value->getOrThrow<std::size_t>("reserved_error_entries"_el)
                : std::min(managerOptions.reservedErrorEntries(), managerOptions.maximumEntries()));
        managerOptions.setReservedErrorBytes(
            value->hasValue("reserved_error_bytes"_el)
                ? unit::ByteLength::fromSizeT(value->getOrThrow<std::size_t>("reserved_error_bytes"_el))
                : std::min(managerOptions.reservedErrorBytes(), managerOptions.maximumBytes()));
        if (value->hasValue("maximum_message_bytes"_el)) {
            managerOptions.setMaximumMessageBytes(
                unit::ByteLength::fromSizeT(value->getOrThrow<std::size_t>("maximum_message_bytes"_el)));
        }
        if (value->hasValue("shutdown_timeout_ms"_el)) {
            managerOptions.setShutdownTimeout(
                time::TimeDelta::milliseconds(value->getOrThrow<int64_t>("shutdown_timeout_ms"_el)));
        }
    }
    result.setManagerOptions(managerOptions);
    for (const auto &section : sectionValue->getList<text::String>("trace_sections"_el)) {
        result.enableTraceSection(LogTraceSection{section});
    }
    if (const auto writers = sectionValue->value("writers"_el); writers != nullptr) {
        for (const auto &writer : *writers) {
            result.addWriter(parseWriter(writer), parseFilter(writer));
        }
    }
    return result;
}

auto LogConfigurationParser::parseWriter(const conf::ValuePtr &value) const -> LogWriterPtr {
    const auto type = value->getTextOrThrow("type"_el);
    const auto compare = text::cCaseInsensitive.asciiComparisonFn();
    if (type.compare("console"_el, compare) == std::strong_ordering::equal) {
        if (_terminal == nullptr) {
            throw err::ParameterError{"A console log writer requires a terminal in the parser."_el, "terminal"_el};
        }
        auto options = ConsoleLogWriterOptions{};
        auto paragraph = options.paragraphOptions();
        paragraph.setLineIndent(value->get<int>("line_indent"_el, paragraph.lineIndent()));
        paragraph.setFirstLineIndent(value->get<int>("first_line_indent"_el, paragraph.firstLineIndent()));
        paragraph.setWrappedLineIndent(value->get<int>("wrapped_line_indent"_el, paragraph.wrappedLineIndent()));
        paragraph.setMaximumLineWraps(value->get<int>("maximum_line_wraps"_el, paragraph.maximumLineWraps()));
        options.setParagraphOptions(std::move(paragraph));
        return LogWriter::createForConsole(_terminal, options);
    }
    if (type.compare("file"_el, compare) == std::strong_ordering::equal) {
        auto options = FileLogWriterOptions{path::Path{value->getTextOrThrow("path"_el)}};
        options.setMode(LogFileMode::fromStringOrThrow(value->getText("mode"_el, "append"_el)));
        options.setRotation(LogFileRotation::fromStringOrThrow(value->getText("rotation"_el, "none"_el)));
        if (value->hasValue("maximum_size"_el)) {
            options.setMaximumSize(unit::ByteLength::fromSizeT(value->getOrThrow<std::size_t>("maximum_size"_el)));
        }
        options.setRetention(value->get<std::size_t>("retention"_el, options.retention()));
        return LogWriter::createForFile(options);
    }
    if (type.compare("syslog"_el, compare) == std::strong_ordering::equal) {
        auto options = SyslogLogWriterOptions{};
        options.setTransport(SyslogTransport::fromStringOrThrow(value->getText("transport"_el, "udp"_el)));
        options.setEndpoint(network::HostEndpoint::fromStringOrThrow(value->getTextOrThrow("endpoint"_el)));
        options.setFacility(value->get<uint8_t>("facility"_el, options.facility()));
        if (value->hasValue("host_name"_el)) {
            options.setHostName(value->getTextOrThrow("host_name"_el));
        }
        if (value->hasValue("application_name"_el)) {
            options.setApplicationName(value->getTextOrThrow("application_name"_el));
        }
        if (value->hasValue("process_id"_el)) {
            options.setProcessId(value->getTextOrThrow("process_id"_el));
        }
        if (value->hasValue("message_id"_el)) {
            options.setMessageId(value->getTextOrThrow("message_id"_el));
        }
        if (value->hasValue("tls_label"_el)) {
            options.setTlsConfigurationLabel(value->getTextOrThrow("tls_label"_el));
        }
        if (value->hasValue("maximum_pending_bytes"_el)) {
            options.setMaximumPendingBytes(
                unit::ByteLength::fromSizeT(value->getOrThrow<std::size_t>("maximum_pending_bytes"_el)));
        }
        return LogWriter::createForSyslog(options);
    }
    throw err::ParameterError{"Unknown built-in log writer type."_el, "type"_el};
}

auto LogConfigurationParser::parseFilter(const conf::ValuePtr &value) -> LogWriterFilter {
    auto levels = LogLevels{};
    const auto configuredLevels = value->getList<text::String>("levels"_el);
    if (configuredLevels.empty()) {
        levels = LogLevel::All;
    } else {
        for (const auto &level : configuredLevels) {
            levels |= LogLevel::fromStringOrThrow(level).toRawValue();
        }
    }
    auto result = LogWriterFilter{levels};
    for (const auto &path : value->getList<text::String>("paths"_el)) {
        result.addPath(LogPath{path});
    }
    return result;
}

}
