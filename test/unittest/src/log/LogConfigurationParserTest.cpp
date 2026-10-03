// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../cterm/support/TerminalTestBackend.hpp"

#include <erbsland/conf/ConfError.hpp>
#include <erbsland/conf/Parser.hpp>
#include <erbsland/conf/vr/Rules.hpp>
#include <erbsland/cterm/Terminal.hpp>
#include <erbsland/Date.hpp>
#include <erbsland/Literals.hpp>
#include <erbsland/log/impl/ActiveLogConfiguration.hpp>
#include <erbsland/log/impl/FileLogWriter.hpp>
#include <erbsland/log/impl/LogLineFormatter.hpp>
#include <erbsland/log/impl/SyslogLogWriter.hpp>
#include <erbsland/LogConfigurationParser.hpp>
#include <erbsland/LogLine.hpp>
#include <erbsland/ParameterFailure.hpp>
#include <erbsland/StringJoins.hpp>
#include <erbsland/StringList.hpp>
#include <erbsland/Time.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <memory>
#include <vector>

using namespace el::text::literals;

TESTED_TARGETS(LogConfigurationParser)
class LogConfigurationParserTest final : public el::UnitTest {
private:
    static constexpr auto cLoggingValue = "logging"_el;

public:
    void testCompiledRulesCoverCompleteSchema() {
        auto parser = el::conf::Parser{};
        const auto document = parser.parseTextOrThrow(
            "[logging]\n"
            "trace_sections: \"database\", \"network\"\n"
            "[.format]\n"
            "pattern: \"{time} {level} {name} {message}\"\n"
            "timestamp_zone: \"utc\"\n"
            "level: \"short_upper\"\n"
            "name: \"full\"\n"
            "name_limit: 0\n"
            "message_truncation: \"none\"\n"
            "message_limit: 0\n"
            "truncation_mark: \"...\"\n"
            "[.queue]\n"
            "maximum_entries: 1\n"
            "maximum_bytes: 1\n"
            "reserved_error_entries: 0\n"
            "reserved_error_bytes: 0\n"
            "maximum_message_bytes: 1\n"
            "shutdown_timeout_ms: 0\n"
            "*[.writers]*\n"
            "type: \"file\"\n"
            "levels: \"trace\", \"information\", \"warning\", \"error\"\n"
            "paths: \"application\", \"library\"\n"
            "path: \"application.log\"\n"
            "mode: \"append\"\n"
            "rotation: \"size\"\n"
            "maximum_size: 1\n"
            "retention: 0\n"
            "endpoint: \"127.0.0.1:514\"\n"
            "transport: \"udp\"\n"
            "facility: 23\n"
            "host_name: \"host\"\n"
            "application_name: \"app\"\n"
            "process_id: \"42\"\n"
            "message_id: \"event\"\n"
            "tls_label: \"log/syslog\"\n"
            "maximum_pending_bytes: 1\n"
            "line_indent: 0\n"
            "first_line_indent: -1\n"
            "wrapped_line_indent: -1\n"
            "maximum_line_wraps: 0\n"_el);

        const auto logging = document->valueOrThrow(cLoggingValue);
        REQUIRE_NOTHROW(el::log::LogConfigurationParser::validationRules()->validate(logging, 0));
        for (const auto &value : *logging) {
            WITH_CONTEXT(requireRuleDocumentation(value));
        }
    }

    void testCompiledRulesEnforceBounds() {
        auto parser = el::conf::Parser{};
        const auto invalidQueue = parser.parseTextOrThrow(
            "[logging.queue]\n"
            "maximum_entries: 0\n"_el);
        REQUIRE_THROWS_AS(
            el::conf::ConfError,
            el::log::LogConfigurationParser::validationRules()->validate(invalidQueue->valueOrThrow(cLoggingValue), 0));

        const auto invalidWriter = parser.parseTextOrThrow(
            "[logging]\n"
            "*[.writers]*\n"
            "type: \"syslog\"\n"
            "facility: 24\n"_el);
        REQUIRE_THROWS_AS(
            el::conf::ConfError,
            el::log::LogConfigurationParser::validationRules()->validate(
                invalidWriter->valueOrThrow(cLoggingValue), 0));
    }

    void testCompiledRulesEnforceEnumChoices() {
        const auto configurations = el::text::StringList{
            "[logging]\n[.format]\ntimestamp_zone: \"invalid\"\n"_el,
            "[logging]\n[.format]\nlevel: \"invalid\"\n"_el,
            "[logging]\n[.format]\nname: \"invalid\"\n"_el,
            "[logging]\n[.format]\nmessage_truncation: \"invalid\"\n"_el,
            "[logging]\n*[.writers]*\ntype: \"invalid\"\n"_el,
            "[logging]\n*[.writers]*\ntype: \"console\"\nlevels: \"invalid\"\n"_el,
            "[logging]\n*[.writers]*\ntype: \"file\"\nmode: \"invalid\"\n"_el,
            "[logging]\n*[.writers]*\ntype: \"file\"\nrotation: \"invalid\"\n"_el,
            "[logging]\n*[.writers]*\ntype: \"syslog\"\ntransport: \"invalid\"\n"_el,
        };
        auto parser = el::conf::Parser{};
        for (const auto &configuration : configurations) {
            const auto document = parser.parseTextOrThrow(configuration);
            REQUIRE_THROWS_AS(
                el::conf::ConfError,
                el::log::LogConfigurationParser::validationRules()->validate(document->valueOrThrow(cLoggingValue), 0));
        }
    }

    void testCompiledRulesReportEnumChoices() {
        auto parser = el::conf::Parser{};
        const auto document = parser.parseTextOrThrow(
            "[logging]\n"
            "[.format]\n"
            "timestamp_zone: \"invalid\"\n"_el);
        try {
            el::log::LogConfigurationParser::validationRules()->validate(document->valueOrThrow(cLoggingValue), 0);
        } catch (const el::conf::ConfError &error) {
            REQUIRE_EQUAL(error.description(), "The text must be one of \"utc\" or \"local\" (case-insensitive)"_el);
            return;
        }
        REQUIRE(false);
    }

    void testParsesSelectedBranch() {
        auto parser = el::conf::Parser{};
        const auto document = parser.parseTextOrThrow(
            "[application.logging]\n"
            "trace_sections: \"database\", \"network\"\n"
            "[.format]\n"
            "pattern: \"{level} {name}: {message}\"\n"
            "timestamp_zone: \"LOCAL\"\n"
            "name: \"LEAF\"\n"
            "[.queue]\n"
            "maximum_entries: 123\n"
            "maximum_bytes: 65536\n"
            "*[.writers]*\n"
            "type: \"FILE\"\n"
            "path: \"application.log\"\n"
            "mode: \"OVERWRITE\"\n"
            "rotation: \"DAILY\"\n"
            "levels: \"INFORMATION\", \"WARNING\", \"ERROR\"\n"
            "paths: \"application\"\n"_el);
        const auto configuration =
            el::log::LogConfigurationParser{}.parse(document->valueOrThrow("application.logging"_el));

        REQUIRE_EQUAL(configuration.lineFormat().pattern(), "{level} {name}: {message}"_el);
        REQUIRE_EQUAL(configuration.lineFormat().timestampZone(), el::log::LogTimestampZone::Local);
        REQUIRE_EQUAL(configuration.lineFormat().nameFormat(), el::log::LogNameFormat::Leaf);
        REQUIRE_EQUAL(configuration.managerOptions().maximumEntries(), std::size_t{123U});
        REQUIRE_EQUAL(configuration.managerOptions().maximumBytes(), el::unit::ByteCount{65536U});
        REQUIRE(configuration.isTraceSectionEnabled(el::log::LogTraceSection{"database"_el}));
        REQUIRE_EQUAL(configuration.writerCount(), std::size_t{1U});
    }

    void testTraceSectionCanonicalization() {
        auto parser = el::conf::Parser{};
        const auto document =
            parser.parseTextOrThrow("[logging]\ntrace_sections: \"Foo-Bar\", \"NETWORK_Details\", \"\", \"\"\n"_el);
        const auto configuration = el::LogConfigurationParser{}.parse(document->valueOrThrow(cLoggingValue));
        REQUIRE_EQUAL(configuration.traceSections().size(), std::size_t{2});
        REQUIRE_EQUAL(configuration.traceSections()[0].toString(), "foo_bar"_el);
        REQUIRE_EQUAL(configuration.traceSections()[1].toString(), "network_details"_el);
        REQUIRE(configuration.isTraceSectionEnabled(el::LogTraceSection{"FOO-BAR"_el}));
    }

    void testTraceSectionDuplicatesAndInvalidNames() {
        const auto entries = el::StringList{
            "\"foo-bar\", \"foo-bar\""_el,
            "\"foo-bar\", \"FOO-BAR\""_el,
            "\"foo-bar\", \"foo_bar\""_el,
            "\"foo-bar\", \"0bad\""_el,
            "\"foo-bar\", \"_bad\""_el,
            "\"foo-bar\", \"bad name\""_el,
            "\"foo-bar\", \"\u00e4\""_el};
        auto parser = el::conf::Parser{};
        for (const auto &entry : entries) {
            const auto document = parser.parseTextOrThrow(el::join("[logging]\ntrace_sections: "_el, entry, "\n"_el));
            const auto logging = document->valueOrThrow(cLoggingValue);
            const auto duplicate = logging->valueOrThrow("trace_sections"_el)->toValueList()[1];
            try {
                (void)el::LogConfigurationParser{}.parse(logging);
            } catch (const el::conf::ConfError &error) {
                REQUIRE_EQUAL(error.category(), el::conf::ConfErrorCategory::Validation);
                REQUIRE_EQUAL(error.location(), duplicate->location().codeLocation());
                REQUIRE_EQUAL(error.namePath(), duplicate->namePath());
                REQUIRE(error.description().contains("trace section"_el));
                continue;
            }
            REQUIRE(false);
        }
    }

    void testTraceSectionLengthBoundaries() {
        constexpr auto maximumName =
            "abcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghij"_el;
        auto parser = el::conf::Parser{};
        const auto validDocument =
            parser.parseTextOrThrow(el::join("[logging]\ntrace_sections: \""_el, maximumName, "\"\n"_el));
        const auto configuration = el::LogConfigurationParser{}.parse(validDocument->valueOrThrow(cLoggingValue));
        REQUIRE_EQUAL(configuration.traceSections().front().toString(), maximumName);
        const auto invalidDocument =
            parser.parseTextOrThrow(el::join("[logging]\ntrace_sections: \""_el, maximumName, "a\"\n"_el));
        REQUIRE_THROWS_AS(
            el::conf::ConfError, el::LogConfigurationParser{}.parse(invalidDocument->valueOrThrow(cLoggingValue)));
    }

    void testWriterFormatsForAllWriterTypesAndGlobalFallback() {
        auto parser = el::conf::Parser{};
        const auto document = parser.parseTextOrThrow(
            "[logging]\n"
            "[.format]\npattern: \"global: {message}\"\nlevel: \"full_upper\"\n"
            "*[logging.writers]*\ntype: \"console\"\n"
            "[.format]\npattern: \"{level}: {message}\"\n"
            "*[logging.writers]*\ntype: \"file\"\npath: \"unused.log\"\n"
            "[.format]\npattern: \"file: {message}\"\n"
            "*[logging.writers]*\ntype: \"syslog\"\nendpoint: \"127.0.0.1:514\"\n"
            "[.format]\npattern: \"syslog: {message}\"\n"
            "*[logging.writers]*\ntype: \"file\"\npath: \"unused-default.log\"\n"
            "*[logging.writers]*\ntype: \"file\"\npath: \"unused-empty.log\"\n[.format]\n"_el);
        const auto terminal = std::make_shared<el::cterm::Terminal>(std::make_shared<TerminalTestBackend>());
        const auto configuration =
            el::log::LogConfigurationParser{terminal}.parse(document->valueOrThrow(cLoggingValue));
        const auto active = el::log::impl::ActiveLogConfiguration{configuration};
        const auto entry = el::log::LogEntry{
            1U,
            el::time::Timestamp{el::time::Date::fromYearMonthDay(2026, 8, 31), el::time::Time{}},
            el::log::LogLevel::Warning,
            el::log::LogPath{},
            "value"_el};
        const auto expected = el::StringList{
            "WRN: value"_el,
            "file: value"_el,
            "syslog: value"_el,
            "global: value"_el,
            "2026-08-31 00:00:00Z WRN - value"_el};
        REQUIRE_EQUAL(configuration.writerCount(), expected.count().toSizeT());
        for (auto index = std::size_t{}; index < expected.count().toSizeT(); ++index) {
            REQUIRE_EQUAL(
                active.writers()[index].formatter->format(entry)->text(),
                expected.getRef(el::ItemIndex::fromInteger(index)));
        }
    }

    void testCompleteWriterFormatSchemaAndRendering() {
        auto parser = el::conf::Parser{};
        const auto document = parser.parseTextOrThrow(
            "[logging]\n*[.writers]*\ntype: \"file\"\npath: \"unused.log\"\n"
            "[.format]\n"
            "pattern: \"{{{level}}} {name}: {message}\"\n"
            "timestamp_zone: \"utc\"\nlevel: \"full_upper\"\nname: \"left_truncated\"\n"
            "name_limit: 4\nmessage_truncation: \"characters\"\nmessage_limit: 3\ntruncation_mark: \"~\"\n"_el);
        const auto logging = document->valueOrThrow(cLoggingValue);
        const auto configuration = el::log::LogConfigurationParser{}.parse(logging);
        for (const auto &value : *logging) {
            WITH_CONTEXT(requireRuleDocumentation(value));
        }
        const auto active = el::log::impl::ActiveLogConfiguration{configuration};
        const auto entry = el::log::LogEntry{
            1U, el::time::Timestamp::now(), el::log::LogLevel::Warning, el::log::LogPath{"app/query"_el}, "abcdef"_el};
        REQUIRE_EQUAL(active.writers()[0U].formatter->format(entry)->text(), "{WARNING} ~ery: ab~"_el);
    }

    void testInvalidWriterFormatFields() {
        const auto fields = el::text::StringList{
            "timestamp_zone: \"invalid\"\n"_el,
            "level: \"invalid\"\n"_el,
            "name: \"invalid\"\n"_el,
            "message_truncation: \"invalid\"\n"_el,
            "name_limit: -1\n"_el,
            "message_limit: -1\n"_el};
        auto parser = el::conf::Parser{};
        for (const auto &field : fields) {
            const auto document = parser.parseTextOrThrow(
                el::join("[logging]\n*[.writers]*\ntype: \"file\"\npath: \"unused.log\"\n[.format]\n"_el, field));
            REQUIRE_THROWS_AS(
                el::conf::ConfError, el::log::LogConfigurationParser{}.parse(document->valueOrThrow(cLoggingValue)));
        }
        const auto document = parser.parseTextOrThrow(
            "[logging]\n*[.writers]*\ntype: \"file\"\npath: \"unused.log\"\n"
            "[.format]\npattern: \"{unknown}\"\n"_el);
        REQUIRE_THROWS_AS(
            el::err::ParameterFailure, el::log::LogConfigurationParser{}.parse(document->valueOrThrow(cLoggingValue)));
    }

    void testSyslogWireFormatsAndTlsDefault() {
        auto options = el::log::SyslogLogWriterOptions{};
        options.setHostName("host"_el).setApplicationName("app"_el).setProcessId("42"_el).setMessageId("event"_el);
        const auto entry = el::log::LogEntry{
            1U,
            el::time::Timestamp{
                el::time::Date::fromYearMonthDay(2026, 8, 31),
                el::time::Time{el::time::Hour{12}, el::time::Minute{34}, el::time::Second{56}}},
            el::log::LogLevel::Warning,
            el::log::LogPath{"application"_el},
            "message"_el};
        const auto line = el::log::LogLine{{{el::log::LogLinePart::Message, "message"_el}}};
        const auto message = el::log::impl::SyslogLogWriter::formatMessage(entry, line, options);

        REQUIRE_EQUAL(message, "<12>1 2026-08-31T12:34:56Z host app 42 event - message"_el);
        REQUIRE_EQUAL(
            el::log::impl::SyslogLogWriter::frameMessage(message),
            "54 <12>1 2026-08-31T12:34:56Z host app 42 event - message"_el);
        REQUIRE_EQUAL(options.tlsConfigurationLabel(), "log/syslog"_el);
    }

    void testSyslogPendingQueueDropsOversizedMessages() {
        auto options = el::log::SyslogLogWriterOptions{};
        options.setEndpoint(el::network::HostEndpoint::fromStringOrThrow("127.0.0.1:514"_el))
            .setMaximumPendingBytes(el::unit::ByteCount{8U});
        auto writer = el::log::impl::SyslogLogWriter{options};
        const auto entry = std::make_shared<el::log::LogEntry>(
            1U, el::time::Timestamp::now(), el::log::LogLevel::Information, el::log::LogPath{}, "message"_el);

        writer.write(
            entry,
            std::make_shared<el::log::LogLine>(
                std::vector<el::log::LogLineSegment>{{el::log::LogLinePart::Message, "too large"_el}}));

        REQUIRE_EQUAL(writer.droppedMessages(), uint64_t{1U});
    }

private:
    void requireRuleDocumentation(const el::conf::ValuePtr &value) {
        REQUIRE(value);
        const auto rule = value->validationRule();
        REQUIRE(rule);
        REQUIRE_FALSE(rule->title().isEmpty());
        REQUIRE_FALSE(rule->description().isEmpty());
        for (const auto &child : *value) {
            WITH_CONTEXT(requireRuleDocumentation(child));
        }
    }
};
