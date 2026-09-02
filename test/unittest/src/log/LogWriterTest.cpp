// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../path/PathTestFixture.hpp"

#include <erbsland/log/impl/FileLogWriter.hpp>
#include <erbsland/log/impl/LastErrorsLogWriter.hpp>
#include <erbsland/log/line/LogLine.hpp>
#include <erbsland/log/LogConfiguration.hpp>
#include <erbsland/log/LogManager.hpp>
#include <erbsland/log/LogStream.hpp>
#include <erbsland/path/PathContent.hpp>
#include <erbsland/path/PathInfo.hpp>
#include <erbsland/path/PathOperations.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/time/Date.hpp>
#include <erbsland/time/DateTime.hpp>
#include <erbsland/time/Time.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <array>
#include <chrono>
#include <filesystem>
#include <memory>
#include <thread>
#include <vector>

using namespace el::text::literals;
using namespace erbsland::test::pathtest;

TESTED_TARGETS(LogWriter)
class LogWriterTest final : public el::UnitTest {
public:
    void testBatchItemSharesImmutableValues() {
        const auto item = el::log::LogWriter::BatchItem{makeEntry(30U, "retained"_el), makeLine("line"_el)};

        REQUIRE_EQUAL(item.entry()->message(), "retained"_el);
        REQUIRE_EQUAL(item.line()->text(), "line"_el);
        REQUIRE_THROWS((el::log::LogWriter::BatchItem{el::log::LogEntryConstPtr{}, makeLine("line"_el)}));
        REQUIRE_THROWS((el::log::LogWriter::BatchItem{makeEntry(30U, "entry"_el), el::log::LogLineConstPtr{}}));
    }

    void testLastErrorsUsesBoundedFifo() {
        const auto manager = el::log::LogManager::create();
        const auto errors = std::make_shared<el::log::impl::LastErrorsLogWriter>(2U);
        auto configuration = el::log::LogConfiguration{};
        configuration.addWriter(errors, el::log::LogWriterFilter{el::log::LogLevel::Error});
        manager->setConfiguration(std::move(configuration));

        manager->rootStream()->error("one"_el);
        manager->rootStream()->error("two"_el);
        manager->rootStream()->error("three"_el);
        manager->shutdown();

        const auto snapshot = errors->snapshot();
        REQUIRE_EQUAL(snapshot.size(), std::size_t{2U});
        REQUIRE_EQUAL(snapshot.front()->message(), "two"_el);
        REQUIRE_EQUAL(snapshot.back()->message(), "three"_el);
    }

    void testFileWriterUsesPathStreams() {
        const auto fixture = PathTestFixture{"log-file-writer"};
        const auto path = fixture.child("application.log");
        const auto writer = std::make_shared<el::log::impl::FileLogWriter>(
            el::log::FileLogWriterOptions{path}.setMode(el::log::LogFileMode::Overwrite));
        const auto manager = el::log::LogManager::create();
        auto configuration = el::log::LogConfiguration{};
        configuration.addWriter(writer);
        manager->setConfiguration(std::move(configuration));

        manager->rootStream()->info("hello"_el);
        manager->shutdown();

        const auto text = path.content().readTextOrThrow();
        REQUIRE_FALSE(text.find("INF - hello"_el).isNoIndex());
    }

    void testFileWriterAppendAndOverwriteModes() {
        const auto fixture = PathTestFixture{"log-file-open-modes"};
        const auto appendPath = fixture.child("append.log");
        const auto overwritePath = fixture.child("overwrite.log");
        appendPath.content().writeTextOrThrow("existing\n"_el);
        overwritePath.content().writeTextOrThrow("existing\n"_el);
        const auto line = makeLine("new"_el);

        auto appendWriter = el::log::impl::FileLogWriter{el::log::FileLogWriterOptions{appendPath}};
        appendWriter.write(makeEntry(30U, "append"_el), line);
        appendWriter.close();
        auto overwriteWriter = el::log::impl::FileLogWriter{
            el::log::FileLogWriterOptions{overwritePath}.setMode(el::log::LogFileMode::Overwrite)};
        overwriteWriter.write(makeEntry(30U, "overwrite"_el), line);
        overwriteWriter.close();

        REQUIRE_EQUAL(appendPath.content().readTextOrThrow(), "existing\nnew\n"_el);
        REQUIRE_EQUAL(overwritePath.content().readTextOrThrow(), "new\n"_el);
    }

    void testFileWriterAcceptsOrderedBatches() {
        const auto fixture = PathTestFixture{"log-file-batch"};
        const auto path = fixture.child("application.log");
        auto writer =
            el::log::impl::FileLogWriter{el::log::FileLogWriterOptions{path}.setMode(el::log::LogFileMode::Overwrite)};
        const auto firstEntry = makeEntry(30U, "first"_el);
        const auto secondEntry = makeEntry(30U, "second"_el);
        const auto firstLine = makeLine("first line"_el);
        const auto secondLine = makeLine("second line"_el);
        const auto batch = std::array{
            el::log::LogWriter::BatchItem{firstEntry, firstLine},
            el::log::LogWriter::BatchItem{secondEntry, secondLine},
        };

        writer.writeBatch(batch);
        writer.close();

        REQUIRE_EQUAL(path.content().readTextOrThrow(), "first line\nsecond line\n"_el);
    }

    void testFileWriterDailyRotationUsesFixedNames() {
        const auto fixture = PathTestFixture{"log-file-daily-rotation"};
        const auto path = fixture.child("application.log");
        auto writer = el::log::impl::FileLogWriter{el::log::FileLogWriterOptions{path}
                .setMode(el::log::LogFileMode::Overwrite)
                .setRotation(el::log::LogFileRotation::Daily)};
        const auto line = makeLine("line"_el);
        writer.write(makeEntry(30U, "first"_el), line);
        writer.write(makeEntry(31U, "second"_el), line);
        writer.close();

        const auto archive = path.withStem("application.2026-08-30"_el);
        REQUIRE(archive.info().exists());
        REQUIRE_FALSE(archive.content().readTextOrThrow().find("line"_el).isNoIndex());
        REQUIRE_FALSE(path.content().readTextOrThrow().find("line"_el).isNoIndex());
    }

    void testFileWriterHourlyAndWeeklyRotationUseFixedNames() {
        const auto fixture = PathTestFixture{"log-file-hourly-weekly-rotation"};
        const auto hourlyPath = fixture.child("hourly.log");
        const auto weeklyPath = fixture.child("weekly.log");
        const auto line = makeLine("line"_el);
        auto hourlyWriter = el::log::impl::FileLogWriter{el::log::FileLogWriterOptions{hourlyPath}
                .setMode(el::log::LogFileMode::Overwrite)
                .setRotation(el::log::LogFileRotation::Hourly)};
        hourlyWriter.write(makeEntry(2026, 8, 30, 10, "first"_el), line);
        hourlyWriter.write(makeEntry(2026, 8, 30, 11, "second"_el), line);
        hourlyWriter.close();

        auto weeklyWriter = el::log::impl::FileLogWriter{el::log::FileLogWriterOptions{weeklyPath}
                .setMode(el::log::LogFileMode::Overwrite)
                .setRotation(el::log::LogFileRotation::Weekly)};
        weeklyWriter.write(makeEntry(2026, 8, 30, 0, "first"_el), line);
        weeklyWriter.write(makeEntry(2026, 8, 31, 0, "second"_el), line);
        weeklyWriter.close();

        REQUIRE(hourlyPath.withStem("hourly.2026-08-30-10"_el).info().exists());
        REQUIRE(weeklyPath.withStem("weekly.2026-08-24"_el).info().exists());
    }

    void testFileWriterTimeRotationEnforcesRetention() {
        const auto fixture = PathTestFixture{"log-file-time-retention"};
        const auto path = fixture.child("application.log");
        auto writer = el::log::impl::FileLogWriter{el::log::FileLogWriterOptions{path}
                .setMode(el::log::LogFileMode::Overwrite)
                .setRotation(el::log::LogFileRotation::Daily)
                .setRetention(2U)};
        const auto line = makeLine("line"_el);
        writer.write(makeEntry(2026, 8, 30, 0, "one"_el), line);
        writer.write(makeEntry(2026, 8, 31, 0, "two"_el), line);
        writer.write(makeEntry(2026, 9, 1, 0, "three"_el), line);
        writer.write(makeEntry(2026, 9, 2, 0, "four"_el), line);
        writer.close();

        REQUIRE_FALSE(path.withStem("application.2026-08-30"_el).info().exists());
        REQUIRE(path.withStem("application.2026-08-31"_el).info().exists());
        REQUIRE(path.withStem("application.2026-09-01"_el).info().exists());
    }

    void testFileWriterSizeRotationEnforcesRetention() {
        const auto fixture = PathTestFixture{"log-file-size-rotation"};
        const auto path = fixture.child("application.log");
        auto writer = el::log::impl::FileLogWriter{el::log::FileLogWriterOptions{path}
                .setMode(el::log::LogFileMode::Overwrite)
                .setRotation(el::log::LogFileRotation::Size)
                .setMaximumSize(el::unit::ByteLength{5U})
                .setRetention(2U)};
        const auto line = makeLine("abc"_el);
        writer.write(makeEntry(30U, "one"_el), line);
        writer.write(makeEntry(30U, "two"_el), line);
        writer.write(makeEntry(30U, "three"_el), line);
        writer.write(makeEntry(30U, "four"_el), line);
        writer.close();

        REQUIRE(path.withStem("application.1"_el).info().exists());
        REQUIRE(path.withStem("application.2"_el).info().exists());
        REQUIRE_FALSE(path.withStem("application.3"_el).info().exists());
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testFileWriterReopensAfterExternalReplacement() {
        const auto fixture = PathTestFixture{"log-file-external-replacement"};
        const auto path = fixture.child("application.log");
        const auto movedPath = fixture.child("collected.log");
        auto writer =
            el::log::impl::FileLogWriter{el::log::FileLogWriterOptions{path}.setMode(el::log::LogFileMode::Overwrite)};
        const auto firstLine = makeLine("first"_el);
        const auto secondLine = makeLine("second"_el);
        writer.write(makeEntry(30U, "first"_el), firstLine);
        writer.flush();
        path.operations().moveToOrThrow(movedPath);
        path.content().writeTextOrThrow("replacement\n"_el);

        std::this_thread::sleep_for(std::chrono::milliseconds{1100});
        writer.write(makeEntry(30U, "second"_el), secondLine);
        writer.close();

        REQUIRE_EQUAL(movedPath.content().readTextOrThrow(), "first\n"_el);
        REQUIRE_EQUAL(path.content().readTextOrThrow(), "replacement\nsecond\n"_el);
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testFileWriterRecoversAfterDeletionAndReconstructedDirectory() {
        const auto fixture = PathTestFixture{"log-file-recovery"};
        const auto path = fixture.child("application.log");
        auto writer =
            el::log::impl::FileLogWriter{el::log::FileLogWriterOptions{path}.setMode(el::log::LogFileMode::Overwrite)};
        const auto firstLine = makeLine("first"_el);
        const auto secondLine = makeLine("second"_el);
        writer.write(makeEntry(30U, "first"_el), firstLine);
        writer.flush();
        path.operations().removeOrThrow();
        std::this_thread::sleep_for(std::chrono::milliseconds{1100});
        writer.write(makeEntry(30U, "second"_el), secondLine);
        writer.close();
        REQUIRE_EQUAL(path.content().readTextOrThrow(), "second\n"_el);

        const auto blockedParent = fixture.child("blocked");
        const auto recoveredPath = fixture.child("blocked/recovered.log");
        blockedParent.content().writeTextOrThrow("not-a-directory"_el);
        auto recoveringWriter = el::log::impl::FileLogWriter{
            el::log::FileLogWriterOptions{recoveredPath}.setMode(el::log::LogFileMode::Overwrite)};
        REQUIRE_THROWS(recoveringWriter.write(makeEntry(30U, "failed"_el), firstLine));
        blockedParent.operations().removeOrThrow();
        REQUIRE_THROWS(recoveringWriter.write(makeEntry(30U, "waiting"_el), firstLine));
        std::this_thread::sleep_for(std::chrono::milliseconds{120});
        recoveringWriter.write(makeEntry(30U, "recovered"_el), secondLine);
        recoveringWriter.close();
        REQUIRE_EQUAL(recoveredPath.content().readTextOrThrow(), "second\n"_el);
    }

private:
    [[nodiscard]] static auto makeEntry(const unsigned day, const el::text::String &message)
        -> el::log::LogEntryConstPtr {
        return makeEntry(2026, 8, day, 0, message);
    }

    [[nodiscard]] static auto makeEntry(
        const int year, const unsigned month, const unsigned day, const unsigned hour, const el::text::String &message)
        -> el::log::LogEntryConstPtr {
        return std::make_shared<el::log::LogEntry>(
            1U,
            el::time::DateTime{
                el::time::Date::fromYearMonthDay(year, month, day),
                el::time::Time{el::time::Hour{hour}, el::time::Minute{}, el::time::Second{}}},
            el::log::LogLevel::Information,
            el::log::LogPath{},
            message);
    }

    [[nodiscard]] static auto makeLine(const el::text::String &message) -> el::log::LogLineConstPtr {
        return std::make_shared<el::log::LogLine>(
            std::vector<el::log::LogLineSegment>{{el::log::LogLinePart::Message, message}});
    }
};
