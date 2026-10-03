// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/Literals.hpp>
#include <erbsland/log/impl/ActiveLogConfiguration.hpp>
#include <erbsland/log/impl/LogLineFormatter.hpp>
#include <erbsland/LogWriter.hpp>
#include <erbsland/Timestamp.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <memory>

using namespace el::text::literals;

TESTED_TARGETS(ActiveLogConfiguration ActiveLogWriter LogLineFormatterPtr LogLineFormatterConstPtr)
class ActiveLogConfigurationTest final : public el::UnitTest {
public:
    void testSharesDefaultFormatterAndCopiesIndependentRuntimeRoutes() {
        auto global = el::log::LogLineFormat{};
        global.setPattern("default: {message}"_el);
        auto custom = el::log::LogLineFormat{};
        custom.setPattern("custom: {message}"_el);
        const auto writer = el::log::LogWriter::createForSyslog();
        auto filter = el::log::LogWriterFilter{}.setAcceptedLevels(el::log::LogLevel::Warning);
        filter.addAcceptedPath(el::log::LogPath{"app"_el});
        auto configuration = el::log::LogConfiguration{};
        configuration.setLineFormat(global)
            .addWriter(writer)
            .addWriter(writer, filter, custom)
            .addWriter(writer)
            .addWriter(writer, {}, custom);
        const auto active = el::log::impl::ActiveLogConfiguration{configuration};
        configuration.setLineFormat(el::log::LogLineFormat{});
        global.setPattern("changed global"_el);
        custom.setPattern("changed custom"_el);
        filter.setAcceptedLevels(el::log::LogLevel::Error);

        const auto &routes = active.writers();
        REQUIRE_EQUAL(routes.size(), std::size_t{4U});
        REQUIRE(routes[0].formatter);
        REQUIRE(routes[0].formatter == routes[2].formatter);
        REQUIRE(routes[0].formatter != routes[1].formatter);
        REQUIRE(routes[1].formatter != routes[3].formatter);
        REQUIRE_EQUAL(active.uniqueWriters().size(), std::size_t{1U});
        REQUIRE(active.uniqueWriters()[0] == writer);
        REQUIRE(routes[1].writer == writer);
        REQUIRE(routes[1].filter.accepts(el::log::LogLevel::Warning, el::log::LogPath{"app/query"_el}));
        REQUIRE_FALSE(routes[1].filter.accepts(el::log::LogLevel::Error, el::log::LogPath{"app/query"_el}));
        REQUIRE_FALSE(routes[1].filter.accepts(el::log::LogLevel::Warning, el::log::LogPath{"other"_el}));
        const auto entry = el::log::LogEntry{
            1U, el::time::Timestamp::now(), el::log::LogLevel::Warning, el::log::LogPath{}, "value"_el};
        REQUIRE_EQUAL(routes[0].formatter->format(entry)->text(), "default: value"_el);
        REQUIRE_EQUAL(routes[1].formatter->format(entry)->text(), "custom: value"_el);
    }

    void testCopiesLimitsAndTracePolicyIndependently() {
        auto configuration = el::log::LogConfiguration{};
        auto limits = el::log::LogManagerOptions{};
        limits.setMaximumEntries(4096U);
        auto filter = el::log::LogWriterFilter{}.setAcceptedLevels(el::log::LogLevel::Trace);
        filter.addAcceptedPath(el::log::LogPath{"app"_el});
        configuration.setManagerOptions(limits)
            .enableTraceSection(el::log::LogTraceSection{"diagnostics"_el})
            .addWriter(el::log::LogWriter::createForSyslog(), filter);
        const auto active = el::log::impl::ActiveLogConfiguration{configuration};
        configuration.setManagerOptions(el::log::LogManagerOptions{})
            .enableTraceSection(el::log::LogTraceSection{"later"_el});

        REQUIRE_EQUAL(active.managerOptions().maximumEntries(), std::size_t{4096U});
        REQUIRE(active.isTraceSectionEnabled({}));
        REQUIRE(active.isTraceSectionEnabled(el::log::LogTraceSection{"diagnostics"_el}));
        REQUIRE(active.isTraceSectionEnabled(el::log::LogTraceSection{"Diagnostics"_el}));
        REQUIRE_FALSE(active.isTraceSectionEnabled(el::log::LogTraceSection{"later"_el}));
    }

    void testUniqueWriterOrderAndEmptyConfiguration() {
        const auto empty = el::log::impl::ActiveLogConfiguration{el::log::LogConfiguration{}};
        REQUIRE(empty.writers().empty());
        REQUIRE(empty.uniqueWriters().empty());
        REQUIRE(empty.isTraceSectionEnabled({}));
        const auto first = el::log::LogWriter::createForSyslog();
        const auto second = el::log::LogWriter::createForSyslog();
        auto configuration = el::log::LogConfiguration{};
        configuration.addWriter(first).addWriter(second).addWriter(first);
        const auto active = el::log::impl::ActiveLogConfiguration{configuration};
        REQUIRE_EQUAL(active.uniqueWriters().size(), std::size_t{2U});
        REQUIRE(active.uniqueWriters()[0] == first);
        REQUIRE(active.uniqueWriters()[1] == second);
    }

    void testFormatterPointersCanOutliveActiveConfiguration() {
        auto formatter = el::log::impl::LogLineFormatterConstPtr{};
        {
            auto configuration = el::log::LogConfiguration{};
            auto format = el::log::LogLineFormat{};
            format.setPattern("retained: {message}"_el);
            configuration.addWriter(el::log::LogWriter::createForSyslog(), {}, format);
            const auto active = el::log::impl::ActiveLogConfiguration{configuration};
            formatter = active.writers()[0].formatter;
        }
        const auto entry = el::log::LogEntry{
            1U, el::time::Timestamp::now(), el::log::LogLevel::Information, el::log::LogPath{}, "value"_el};
        REQUIRE_EQUAL(formatter->format(entry)->text(), "retained: value"_el);
    }
};
