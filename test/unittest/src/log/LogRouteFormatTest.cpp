// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/Literals.hpp>
#include <erbsland/LogConfiguration.hpp>
#include <erbsland/LogicFailure.hpp>
#include <erbsland/LogLine.hpp>
#include <erbsland/LogManager.hpp>
#include <erbsland/LogStream.hpp>
#include <erbsland/LogWriter.hpp>
#include <erbsland/RuntimeError.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <memory>
#include <utility>
#include <vector>

using namespace el::text::literals;

namespace erbsland::test::logroute {

/// Retains immutable values for route-format delivery assertions.
/// @tested{LogRouteFormatTest}
class RecordingWriter final : public el::log::LogWriter {
public: // implement LogWriter
    void write(const el::log::LogEntryConstPtr &entry, const el::log::LogLineConstPtr &line) override {
        entries.push_back(entry);
        lines.push_back(line);
    }

public:
    std::vector<el::log::LogEntryConstPtr> entries; ///< Delivered entries.
    std::vector<el::log::LogLineConstPtr> lines;    ///< Delivered lines.
};

/// Throws during delivery to verify independent route failure containment.
/// @tested{LogRouteFormatTest}
class ThrowingWriter final : public el::log::LogWriter {
public: // implement LogWriter
    void write(const el::log::LogEntryConstPtr &, const el::log::LogLineConstPtr &) override {
        throw el::err::RuntimeError{"delivery failure"_el};
    }
};

}

TESTED_TARGETS(LogConfiguration LogManagerData LogWriterBinding ActiveLogConfiguration)
class LogRouteFormatTest final : public el::UnitTest {
    using RecordingWriter = erbsland::test::logroute::RecordingWriter;
    using ThrowingWriter = erbsland::test::logroute::ThrowingWriter;

public:
    void testUnnamedTraceEntriesQueueWithoutWriters() {
        const auto manager = el::log::LogManager::create();
        manager->pause();
        const auto root = manager->rootStream();
        const auto stream = manager->createStream("app"_el);
        REQUIRE(root->traceEnabled());
        REQUIRE(stream->traceEnabled());
        root->trace("root trace"_el);
        stream->trace("stream trace"_el);
        REQUIRE_EQUAL(manager->statistics().acceptedEntries, uint64_t{2U});

        const auto writer = std::make_shared<RecordingWriter>();
        manager->setConfiguration(el::log::LogConfiguration{}.addWriter(writer));
        manager->shutdown();
        REQUIRE_EQUAL(writer->entries.size(), std::size_t{2U});
        REQUIRE_EQUAL(writer->entries[0]->message(), "root trace"_el);
        REQUIRE_EQUAL(writer->entries[1]->message(), "stream trace"_el);
        REQUIRE_EQUAL(manager->statistics().writtenEntries, uint64_t{2U});
    }

    void testTraceSectionAdmissionIsIndependentOfWriterFilters() {
        const auto manager = el::log::LogManager::create();
        const auto writer = std::make_shared<RecordingWriter>();
        const auto section = el::log::LogTraceSection{"diagnostics"_el};
        auto configuration = el::log::LogConfiguration{};
        configuration.enableTraceSection(section).addWriter(
            writer, el::log::LogWriterFilter{}.setAcceptedLevels(el::log::LogLevel::Information));
        manager->setConfiguration(std::move(configuration));
        manager->pause();
        const auto unnamed = manager->createStream("app"_el);
        const auto named = manager->createStream("app"_el, section);
        const auto disabled = manager->createStream("app"_el, el::log::LogTraceSection{"other-diagnostics"_el});
        REQUIRE(unnamed->traceEnabled());
        REQUIRE(named->traceEnabled());
        REQUIRE_FALSE(disabled->traceEnabled());
        unnamed->trace("unnamed"_el);
        named->trace("named"_el);
        disabled->trace("disabled"_el);
        REQUIRE_EQUAL(manager->statistics().acceptedEntries, uint64_t{2U});
        REQUIRE_EQUAL(manager->statistics().writtenEntries, uint64_t{0U});
        REQUIRE(writer->entries.empty());

        auto pathFilter = el::log::LogWriterFilter{}.setAcceptedLevels(el::log::LogLevel::Trace);
        pathFilter.addAcceptedPath(el::log::LogPath{"other"_el});
        configuration = el::log::LogConfiguration{};
        configuration.enableTraceSection(section).addWriter(writer, pathFilter);
        manager->setConfiguration(std::move(configuration));
        REQUIRE(unnamed->traceEnabled());
        REQUIRE(named->traceEnabled());
        unnamed->trace("filtered path"_el);
        named->trace("filtered named path"_el);
        manager->shutdown();
        REQUIRE_EQUAL(manager->statistics().acceptedEntries, uint64_t{4U});
        REQUIRE_EQUAL(manager->statistics().writtenEntries, uint64_t{0U});
        REQUIRE(writer->entries.empty());
    }

    void testTraceSectionRefreshWithoutWriters() {
        const auto manager = el::log::LogManager::create();
        manager->pause();
        const auto section = el::log::LogTraceSection{"diagnostics"_el};
        const auto unnamed = manager->createStream("app"_el);
        const auto named = manager->createStream("app"_el, section);
        REQUIRE(unnamed->traceEnabled());
        REQUIRE_FALSE(named->traceEnabled());
        manager->setConfiguration(el::log::LogConfiguration{}.enableTraceSection(section));
        REQUIRE(unnamed->traceEnabled());
        REQUIRE(named->traceEnabled());
        named->trace("queued while enabled"_el);

        const auto writer = std::make_shared<RecordingWriter>();
        manager->setConfiguration(el::log::LogConfiguration{}.addWriter(writer));
        REQUIRE(unnamed->traceEnabled());
        REQUIRE_FALSE(named->traceEnabled());
        named->trace("disabled"_el);
        unnamed->trace("still enabled"_el);
        REQUIRE_FALSE(el::log::LogStream::createMuted()->traceEnabled());
        manager->shutdown();
        REQUIRE_EQUAL(writer->entries.size(), std::size_t{2U});
        REQUIRE_EQUAL(writer->entries[0]->message(), "queued while enabled"_el);
        REQUIRE_EQUAL(writer->entries[1]->message(), "still enabled"_el);
        REQUIRE_EQUAL(manager->statistics().acceptedEntries, uint64_t{2U});
    }

    void testOverridesFilteringAndSharedDefaultLines() {
        const auto manager = el::log::LogManager::create();
        const auto first = std::make_shared<RecordingWriter>();
        const auto second = std::make_shared<RecordingWriter>();
        const auto custom = std::make_shared<RecordingWriter>();
        auto configuration = el::log::LogConfiguration{};
        configuration.setLineFormat(pattern("default: {message}"_el))
            .addWriter(first)
            .addWriter(second)
            .addWriter(
                custom,
                el::log::LogWriterFilter{}.setAcceptedLevels(el::log::LogLevel::Warning),
                pattern("{level}: {message}"_el));
        manager->setConfiguration(std::move(configuration));
        manager->pause();
        manager->rootStream()->info("first"_el);
        manager->rootStream()->warn("second"_el);
        manager->shutdown();

        REQUIRE_EQUAL(first->lines.size(), std::size_t{2U});
        REQUIRE_EQUAL(second->lines.size(), std::size_t{2U});
        REQUIRE(first->lines[0] == second->lines[0]);
        REQUIRE(first->lines[1] == second->lines[1]);
        REQUIRE_EQUAL(first->lines[0]->text(), "default: first"_el);
        REQUIRE_EQUAL(first->lines[1]->text(), "default: second"_el);
        REQUIRE_EQUAL(custom->lines.size(), std::size_t{1U});
        REQUIRE(custom->entries[0] == first->entries[1]);
        REQUIRE(custom->lines[0] != first->lines[1]);
        REQUIRE_EQUAL(custom->lines[0]->text(), "WRN: second"_el);
        REQUIRE_EQUAL(manager->statistics().writtenEntries, uint64_t{2U});
    }

    void testSameWriterDifferentRoutesAndConfigurationRoundTrip() {
        const auto manager = el::log::LogManager::create();
        const auto writer = std::make_shared<RecordingWriter>();
        auto custom = pattern("first: {message}"_el);
        auto original = el::log::LogConfiguration{};
        original.addWriter(writer, {}, custom).addWriter(writer, {}, pattern("second: {message}"_el));
        auto copied = original;
        original.setLineFormat(pattern("changed"_el));
        custom.setPattern("changed custom"_el);
        manager->setConfiguration(std::move(copied));
        manager->setConfiguration(manager->configuration());
        manager->rootStream()->info("value"_el);
        manager->shutdown();

        REQUIRE_EQUAL(writer->lines.size(), std::size_t{2U});
        REQUIRE(writer->entries[0] == writer->entries[1]);
        REQUIRE_EQUAL(writer->lines[0]->text(), "first: value"_el);
        REQUIRE_EQUAL(writer->lines[1]->text(), "second: value"_el);
        REQUIRE_EQUAL(manager->statistics().writtenEntries, uint64_t{1U});
    }

    void testRetainsUserConfigurationAndExplicitDefaultOverride() {
        const auto manager = el::log::LogManager::create();
        const auto writer = std::make_shared<RecordingWriter>();
        auto original = el::log::LogConfiguration{};
        auto filter = el::log::LogWriterFilter{}.setAcceptedLevels(el::log::LogLevel::Information);
        filter.addAcceptedPath(el::log::LogPath{"app"_el});
        original.setLineFormat(pattern("before: {message}"_el))
            .enableTraceSection(el::log::LogTraceSection{"diagnostics"_el})
            .addWriter(writer, filter)
            .addWriter(writer, filter, pattern("before: {message}"_el));
        manager->setConfiguration(original);
        auto retained = manager->configuration();
        REQUIRE_EQUAL(retained.lineFormat().pattern(), "before: {message}"_el);
        REQUIRE(retained.isTraceSectionEnabled(el::log::LogTraceSection{"diagnostics"_el}));
        REQUIRE_EQUAL(retained.writerCount(), std::size_t{2U});
        retained.setLineFormat(pattern("after: {message}"_el));
        REQUIRE_EQUAL(manager->configuration().lineFormat().pattern(), "before: {message}"_el);
        manager->setConfiguration(std::move(retained));
        manager->pause();
        manager->createStream("other"_el)->info("filtered"_el);
        manager->createStream("app"_el)->info("value"_el);
        manager->shutdown();

        REQUIRE_EQUAL(writer->lines.size(), std::size_t{2U});
        REQUIRE_EQUAL(writer->lines[0]->text(), "after: value"_el);
        REQUIRE_EQUAL(writer->lines[1]->text(), "before: value"_el);
    }

    void testReusesWriterWithReplacementRouteFormat() {
        const auto manager = el::log::LogManager::create();
        const auto writer = std::make_shared<RecordingWriter>();
        manager->setConfiguration(el::log::LogConfiguration{}.addWriter(writer, {}, pattern("old: {message}"_el)));
        manager->pause();
        manager->rootStream()->info("queued"_el);
        manager->setConfiguration(el::log::LogConfiguration{}.addWriter(writer, {}, pattern("new: {message}"_el)));
        manager->shutdown();
        REQUIRE_EQUAL(writer->lines.size(), std::size_t{1U});
        REQUIRE_EQUAL(writer->lines[0]->text(), "new: queued"_el);
    }

    void testBindingFailureKeepsOldActiveConfigurationAndReleasesClaims() {
        const auto manager = el::log::LogManager::create();
        const auto otherManager = el::log::LogManager::create();
        const auto original = std::make_shared<RecordingWriter>();
        const auto claimed = std::make_shared<RecordingWriter>();
        const auto occupied = std::make_shared<RecordingWriter>();
        manager->setConfiguration(el::log::LogConfiguration{}.addWriter(original, {}, pattern("old: {message}"_el)));
        otherManager->setConfiguration(el::log::LogConfiguration{}.addWriter(occupied));
        auto replacement = el::log::LogConfiguration{};
        replacement.addWriter(claimed, {}, pattern("new: {message}"_el)).addWriter(occupied);
        manager->pause();
        manager->rootStream()->info("queued"_el);
        REQUIRE_THROWS_AS(el::err::LogicFailure, manager->setConfiguration(std::move(replacement)));
        manager->shutdown();
        otherManager->setConfiguration(el::log::LogConfiguration{}.addWriter(claimed));
        otherManager->shutdown();

        REQUIRE_EQUAL(original->lines.size(), std::size_t{1U});
        REQUIRE_EQUAL(original->lines[0]->text(), "old: queued"_el);
        REQUIRE(claimed->lines.empty());
    }

    void testCustomRouteFailureDoesNotPreventOtherRoutes() {
        const auto manager = el::log::LogManager::create();
        const auto healthy = std::make_shared<RecordingWriter>();
        auto configuration = el::log::LogConfiguration{};
        configuration.addWriter(std::make_shared<ThrowingWriter>(), {}, pattern("failing: {message}"_el))
            .addWriter(healthy, {}, pattern("healthy: {message}"_el));
        manager->setConfiguration(std::move(configuration));
        manager->pause();
        manager->rootStream()->info("first"_el);
        manager->rootStream()->info("second"_el);
        manager->shutdown();
        REQUIRE_EQUAL(healthy->lines.size(), std::size_t{2U});
        REQUIRE_EQUAL(healthy->lines[0]->text(), "healthy: first"_el);
        REQUIRE_EQUAL(healthy->lines[1]->text(), "healthy: second"_el);
        REQUIRE_EQUAL(manager->statistics().writtenEntries, uint64_t{2U});
        REQUIRE_EQUAL(manager->statistics().writerFailures, uint64_t{1U});
    }

private:
    [[nodiscard]] static auto pattern(el::text::String value) -> el::log::LogLineFormat {
        auto result = el::log::LogLineFormat{};
        result.setPattern(std::move(value));
        return result;
    }
};
