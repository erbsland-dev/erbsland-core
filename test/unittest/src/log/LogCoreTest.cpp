// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/err/ParameterError.hpp>
#include <erbsland/log/impl/LastErrorsLogWriter.hpp>
#include <erbsland/log/line/LogLine.hpp>
#include <erbsland/log/LogConfiguration.hpp>
#include <erbsland/log/LogFileMode.hpp>
#include <erbsland/log/LogFileRotation.hpp>
#include <erbsland/log/LogLevel.hpp>
#include <erbsland/log/LogLevelFormat.hpp>
#include <erbsland/log/LogManager.hpp>
#include <erbsland/log/LogMessageTruncation.hpp>
#include <erbsland/log/LogNameFormat.hpp>
#include <erbsland/log/LogPath.hpp>
#include <erbsland/log/LogStream.hpp>
#include <erbsland/log/LogTimestampZone.hpp>
#include <erbsland/log/LogWriter.hpp>
#include <erbsland/log/SyslogTransport.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/StringEditor.hpp>
#include <erbsland/text/StringList.hpp>
#include <erbsland/time/IsoTimeFormat.hpp>
#include <erbsland/time/TimeZone.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <vector>

using namespace el::text::literals;

namespace erbsland::test::logtest {

class CaptureWriter final : public el::log::LogWriter {
public:
    void write(const el::log::LogEntryConstPtr &entry, const el::log::LogLineConstPtr &line) override {
        const auto lock = std::scoped_lock{mutex};
        entries.push_back(entry);
        lines.push_back(line->text());
        condition.notify_all();
    }

    [[nodiscard]] auto waitForEntries(const std::size_t count) -> bool {
        auto lock = std::unique_lock{mutex};
        return condition.wait_for(
            lock, std::chrono::milliseconds{100}, [this, count]() -> bool { return entries.size() >= count; });
    }

    std::mutex mutex;
    std::condition_variable condition;
    std::vector<el::log::LogEntryConstPtr> entries;
    std::vector<el::text::String> lines;
};

class FailingWriter final : public el::log::LogWriter {
public:
    void write(const el::log::LogEntryConstPtr &, const el::log::LogLineConstPtr &) override {
        throw std::runtime_error{"failure"};
    }
};

class BatchCaptureWriter final : public el::log::LogWriter {
public:
    void write(const el::log::LogEntryConstPtr &entry, const el::log::LogLineConstPtr &line) override {
        entries.push_back(entry);
        lines.push_back(line);
    }

    void writeBatch(const Batch batch) override {
        batchSizes.push_back(batch.size());
        for (const auto &item : batch) {
            entries.push_back(item.entry());
            lines.push_back(item.line());
        }
    }

    std::vector<std::size_t> batchSizes;
    std::vector<el::log::LogEntryConstPtr> entries;
    std::vector<el::log::LogLineConstPtr> lines;
};

class BlockingWriter final : public el::log::LogWriter {
public:
    void write(const el::log::LogEntryConstPtr &, const el::log::LogLineConstPtr &) override {
        auto lock = std::unique_lock{mutex};
        entered = true;
        condition.notify_all();
        condition.wait(lock, [this]() -> bool { return released; });
        ++written;
    }

    void waitUntilEntered() {
        auto lock = std::unique_lock{mutex};
        condition.wait(lock, [this]() -> bool { return entered; });
    }

    void release() {
        const auto lock = std::scoped_lock{mutex};
        released = true;
        condition.notify_all();
    }

    std::mutex mutex;
    std::condition_variable condition;
    bool entered{false};
    bool released{false};
    std::size_t written{0U};
};

}

using namespace erbsland::test::logtest;

TESTED_TARGETS(
    LogConfiguration LogEntry LogFileMode LogFileRotation LogLevel LogLevelFormat LogLine LogLineFormat LogLineFormatter
        LogLinePart LogManager LogManagerData LogManagerOptions LogManagerStatistics LogMessageTruncation LogNameFormat
            LogPath LogStream LogTimestampZone LogTraceSection LogWriterBinding LogWriterFilter SyslogTransport)
class LogCoreTest final : public el::UnitTest {
public:
    void testConfigurationValueConversions() {
        REQUIRE_EQUAL(el::log::LogLevel::Information, el::log::LogLevel::fromStringOrThrow("INFO"_el));
        REQUIRE_EQUAL(el::log::LogLevel{el::log::LogLevel::Warning}.toString(), "warning"_el);
        REQUIRE_FALSE(el::log::LogLevel::fromString("debug"_el).has_value());

        REQUIRE_EQUAL(el::log::LogFileMode::Overwrite, el::log::LogFileMode::fromStringOrThrow("OVERWRITE"_el));
        REQUIRE_EQUAL(el::log::LogFileRotation{el::log::LogFileRotation::Weekly}.toString(), "weekly"_el);
        REQUIRE_EQUAL(el::log::LogFileRotation::Weekly, el::log::LogFileRotation::fromStringOrThrow("WEEKLY"_el));
        REQUIRE_EQUAL(el::log::LogTimestampZone::Local, el::log::LogTimestampZone::fromStringOrThrow("LOCAL"_el));
        REQUIRE_EQUAL(el::log::LogLevelFormat::FullUpper, el::log::LogLevelFormat::fromStringOrThrow("FULL_UPPER"_el));
        REQUIRE_EQUAL(el::log::LogNameFormat{el::log::LogNameFormat::HeadAndLeaf}.toString(), "head_and_leaf"_el);
        REQUIRE_EQUAL(
            el::log::LogNameFormat::HeadAndLeaf, el::log::LogNameFormat::fromStringOrThrow("HEAD_AND_LEAF"_el));
        REQUIRE_EQUAL(
            el::log::LogMessageTruncation::CharacterCount,
            el::log::LogMessageTruncation::fromStringOrThrow("CHARACTERS"_el));
        REQUIRE_EQUAL(el::log::LogLinePart::Message, el::log::LogLinePart::fromStringOrThrow("message"_el));
        REQUIRE_EQUAL(el::log::SyslogTransport{el::log::SyslogTransport::Tls}.toString(), "tls"_el);
        REQUIRE_EQUAL(el::log::SyslogTransport::Tls, el::log::SyslogTransport::fromStringOrThrow("TLS"_el));
        REQUIRE_FALSE(el::log::SyslogTransport::fromString("serial"_el).has_value());
    }

    void testConfigurationValueStrings() {
        REQUIRE_EQUAL(el::log::LogTimestampZone::allStrings(), (el::text::StringList{"utc"_el, "local"_el}));
        REQUIRE_EQUAL(
            el::log::LogLevelFormat::allStrings(),
            (el::text::StringList{"short_upper"_el, "short_lower"_el, "full_lower"_el, "full_upper"_el}));
        REQUIRE_EQUAL(
            el::log::LogNameFormat::allStrings(),
            (el::text::StringList{"full"_el, "leaf"_el, "head_and_leaf"_el, "left_truncated"_el}));
        REQUIRE_EQUAL(
            el::log::LogMessageTruncation::allStrings(),
            (el::text::StringList{"none"_el, "first_line"_el, "characters"_el, "total_line"_el}));
        REQUIRE_EQUAL(
            el::log::LogLevel::allStrings(),
            (el::text::StringList{
                "trace"_el, "information"_el, "info"_el, "warning"_el, "warn"_el, "error"_el, "all"_el}));
        REQUIRE_EQUAL(el::log::LogFileMode::allStrings(), (el::text::StringList{"overwrite"_el, "append"_el}));
        REQUIRE_EQUAL(
            el::log::LogFileRotation::allStrings(),
            (el::text::StringList{"none"_el, "hourly"_el, "daily"_el, "weekly"_el, "size"_el}));
        REQUIRE_EQUAL(el::log::SyslogTransport::allStrings(), (el::text::StringList{"udp"_el, "tcp"_el, "tls"_el}));
    }

    void testManagerOptionsUseChainableAccessors() {
        auto options = el::log::LogManagerOptions{};
        const auto &result = options.setMaximumEntries(32U)
                                 .setMaximumBytes(el::unit::ByteLength{8192U})
                                 .setReservedErrorEntries(4U)
                                 .setReservedErrorBytes(el::unit::ByteLength{1024U})
                                 .setMaximumMessageBytes(el::unit::ByteLength{512U})
                                 .setShutdownTimeout(el::time::TimeDelta::milliseconds(50));
        REQUIRE(&result == &options);
        REQUIRE_EQUAL(options.maximumEntries(), std::size_t{32U});
        REQUIRE_EQUAL(options.maximumBytes(), el::unit::ByteLength{8192U});
        REQUIRE_EQUAL(options.reservedErrorEntries(), std::size_t{4U});
        REQUIRE_EQUAL(options.reservedErrorBytes(), el::unit::ByteLength{1024U});
        REQUIRE_EQUAL(options.maximumMessageBytes(), el::unit::ByteLength{512U});
        REQUIRE_EQUAL(options.shutdownTimeout(), el::time::TimeDelta::milliseconds(50));
    }

    void testPathValidationAndHierarchy() {
        const auto root = el::log::LogPath{};
        const auto app = el::log::LogPath{"app"_el};
        const auto connection = el::log::LogPath{"app/network/connection_1"_el};

        REQUIRE(root.contains(connection));
        REQUIRE(app.contains(connection));
        REQUIRE_FALSE(connection.contains(app));
        REQUIRE_THROWS_AS(el::err::ParameterError, el::log::LogPath{"App"_el});
        REQUIRE_THROWS_AS(el::err::ParameterError, el::log::LogPath{"app//network"_el});
        REQUIRE_THROWS_AS(el::err::ParameterError, el::log::LogPath{"app/-network"_el});
        REQUIRE_THROWS_AS(el::err::ParameterError, el::log::LogPath{"app/network-"_el});

        auto segments = el::text::StringEditor{};
        for (auto index = 0; index < 16; ++index) {
            if (index != 0) {
                segments.append(U'/');
            }
            segments.append(U'a');
        }
        REQUIRE_NOTHROW(el::log::LogPath{segments});
        segments.append("/a"_el);
        REQUIRE_THROWS_AS(el::err::ParameterError, el::log::LogPath{segments});

        auto maximumLength = el::text::StringEditor{};
        for (auto index = 0; index < 255; ++index) {
            maximumLength.append(U'a');
        }
        REQUIRE_NOTHROW(el::log::LogPath{maximumLength});
        maximumLength.append(U'a');
        REQUIRE_THROWS_AS(el::err::ParameterError, el::log::LogPath{maximumLength});
    }

    void testTraceFlagRequiresRouteAndSection() {
        const auto manager = el::log::LogManager::create();
        const auto stream = manager->createStream("app/network"_el, el::log::LogTraceSection{"NetworkDetails"_el});
        REQUIRE_FALSE(stream->traceEnabled());

        const auto errors = std::make_shared<el::log::impl::LastErrorsLogWriter>();
        auto configuration = el::log::LogConfiguration{};
        configuration.addWriter(errors);
        manager->setConfiguration(configuration);
        REQUIRE_FALSE(stream->traceEnabled());

        configuration.enableTraceSection(el::log::LogTraceSection{"NetworkDetails"_el});
        manager->setConfiguration(configuration);
        REQUIRE(stream->traceEnabled());
        manager->shutdown();
    }

    void testMessagesAreSanitizedAndTimestampedAtCreation() {
        const auto manager = el::log::LogManager::create();
        const auto errors = std::make_shared<el::log::impl::LastErrorsLogWriter>();
        auto configuration = el::log::LogConfiguration{};
        configuration.addWriter(errors, el::log::LogWriterFilter{el::log::LogLevel::Error});
        manager->setConfiguration(std::move(configuration));

        auto message = el::text::StringEditor{"first\nsecond"_el};
        message.append(U'\x1b');
        manager->rootStream()->error(message);
        manager->shutdown();

        const auto snapshot = errors->snapshot();
        REQUIRE_EQUAL(snapshot.size(), std::size_t{1U});
        REQUIRE(snapshot.front()->timestamp().isValid());
        REQUIRE_EQUAL(snapshot.front()->timestamp().timeOffset().toSeconds(), el::time::Seconds{});
        REQUIRE_EQUAL(snapshot.front()->message(), "first\nsecond\\u{1b}"_el);
    }

    void testVariadicEntryCreationUsesTextPrintConventions() {
        const auto manager = el::log::LogManager::create();
        const auto writer = std::make_shared<CaptureWriter>();
        manager->setConfiguration(el::log::LogConfiguration{}.addWriter(writer));
        manager->rootStream()->info("value="_el, 42, ", state="_el, true);
        manager->shutdown();

        REQUIRE_EQUAL(writer->entries.size(), std::size_t{1U});
        REQUIRE_EQUAL(writer->entries.front()->message(), "value=42, state=true"_el);
    }

    void testPausedEntriesUseReplacementConfiguration() {
        const auto manager = el::log::LogManager::create();
        const auto oldWriter = std::make_shared<CaptureWriter>();
        const auto newWriter = std::make_shared<CaptureWriter>();
        manager->setConfiguration(el::log::LogConfiguration{}.addWriter(oldWriter));
        manager->pause();
        manager->rootStream()->info("queued"_el);
        manager->setConfiguration(el::log::LogConfiguration{}.addWriter(newWriter));
        manager->resume();
        manager->shutdown();

        REQUIRE(oldWriter->entries.empty());
        REQUIRE_EQUAL(newWriter->entries.size(), std::size_t{1U});
        REQUIRE_EQUAL(newWriter->entries.front()->message(), "queued"_el);
    }

    void testConfigurationBarrierStopsWorkerAfterCurrentEntry() {
        const auto manager = el::log::LogManager::create();
        const auto oldWriter = std::make_shared<BlockingWriter>();
        const auto newWriter = std::make_shared<CaptureWriter>();
        manager->setConfiguration(el::log::LogConfiguration{}.addWriter(oldWriter));
        manager->rootStream()->info("current"_el);
        oldWriter->waitUntilEntered();
        manager->rootStream()->info("queued"_el);

        auto reconfigureThread =
            std::thread{[&]() -> void { manager->setConfiguration(el::log::LogConfiguration{}.addWriter(newWriter)); }};
        std::this_thread::sleep_for(std::chrono::milliseconds{10});
        oldWriter->release();
        reconfigureThread.join();
        manager->shutdown();

        REQUIRE_EQUAL(oldWriter->written, std::size_t{1U});
        REQUIRE_EQUAL(newWriter->entries.size(), std::size_t{1U});
        REQUIRE_EQUAL(newWriter->entries.front()->message(), "queued"_el);
    }

    void testShutdownDeadlineDropsEntriesWaitingBehindCurrentWrite() {
        auto options = el::log::LogManagerOptions{};
        options.setShutdownTimeout(el::time::TimeDelta::milliseconds(20));
        const auto manager = el::log::LogManager::create(options);
        const auto writer = std::make_shared<BlockingWriter>();
        auto configuration = el::log::LogConfiguration{};
        configuration.setManagerOptions(options).addWriter(writer);
        manager->setConfiguration(std::move(configuration));
        manager->rootStream()->info("current"_el);
        writer->waitUntilEntered();
        manager->rootStream()->info("queued-one"_el);
        manager->rootStream()->info("queued-two"_el);

        auto releaseThread = std::thread{[&]() -> void {
            std::this_thread::sleep_for(std::chrono::milliseconds{40});
            writer->release();
        }};
        manager->shutdown();
        releaseThread.join();

        const auto statistics = manager->statistics();
        REQUIRE_EQUAL(statistics.acceptedEntries, uint64_t{3U});
        REQUIRE_EQUAL(statistics.writtenEntries, uint64_t{1U});
        REQUIRE_EQUAL(statistics.droppedEntries, uint64_t{2U});
    }

    void testRouteMatchingUsesCompleteSegments() {
        const auto manager = el::log::LogManager::create();
        const auto writer = std::make_shared<CaptureWriter>();
        auto filter = el::log::LogWriterFilter{el::log::LogLevel::Information};
        filter.addPath(el::log::LogPath{"app"_el});
        manager->setConfiguration(el::log::LogConfiguration{}.addWriter(writer, filter));
        manager->createStream("app/worker"_el)->info("accepted"_el);
        manager->createStream("apple"_el)->info("rejected"_el);
        manager->createStream("app/worker"_el)->warn("wrong-level"_el);
        manager->shutdown();

        REQUIRE_EQUAL(writer->entries.size(), std::size_t{1U});
        REQUIRE_EQUAL(writer->entries.front()->message(), "accepted"_el);
    }

    void testQueueReservationsAndStatistics() {
        auto options = el::log::LogManagerOptions{};
        options.setMaximumEntries(3U)
            .setReservedErrorEntries(1U)
            .setMaximumBytes(el::unit::ByteLength{1024U * 1024U})
            .setReservedErrorBytes({});
        const auto manager = el::log::LogManager::create(options);
        const auto writer = std::make_shared<BlockingWriter>();
        auto configuration = el::log::LogConfiguration{};
        configuration.setManagerOptions(options).addWriter(writer);
        manager->setConfiguration(std::move(configuration));
        manager->rootStream()->info("one"_el);
        writer->waitUntilEntered();
        manager->rootStream()->info("two"_el);
        manager->rootStream()->info("three"_el);
        manager->rootStream()->info("dropped-information"_el);
        manager->rootStream()->warn("reserved"_el);
        manager->rootStream()->error("dropped-error"_el);
        writer->release();
        manager->shutdown();

        const auto statistics = manager->statistics();
        REQUIRE_EQUAL(statistics.acceptedEntries, uint64_t{4U});
        REQUIRE_EQUAL(statistics.droppedEntries, uint64_t{2U});
        REQUIRE_EQUAL(statistics.writtenEntries, uint64_t{4U});
        REQUIRE_EQUAL(writer->written, std::size_t{4U});
    }

    void testPausedManagerDrainsWhenReservedCapacityIsReached() {
        auto options = el::log::LogManagerOptions{};
        options.setMaximumEntries(3U).setReservedErrorEntries(1U);
        const auto manager = el::log::LogManager::create(options);
        const auto writer = std::make_shared<CaptureWriter>();
        manager->setConfiguration(el::log::LogConfiguration{}.setManagerOptions(options).addWriter(writer));
        manager->pause();
        manager->rootStream()->info("one"_el);
        manager->rootStream()->info("two"_el);

        REQUIRE(writer->waitForEntries(1U));
        REQUIRE_EQUAL(manager->statistics().queuedEntries, std::size_t{});
        manager->shutdown();
        REQUIRE_EQUAL(writer->entries.size(), std::size_t{2U});
    }

    void testManagerDeliversQueuedEntriesInBatches() {
        const auto manager = el::log::LogManager::create();
        const auto writer = std::make_shared<BatchCaptureWriter>();
        manager->setConfiguration(el::log::LogConfiguration{}.addWriter(writer));
        manager->pause();
        manager->rootStream()->info("one"_el);
        manager->rootStream()->info("two"_el);
        manager->rootStream()->info("three"_el);
        manager->resume();
        manager->shutdown();

        REQUIRE_EQUAL(writer->batchSizes, std::vector<std::size_t>{3U});
        REQUIRE_EQUAL(writer->entries.size(), std::size_t{3U});
        REQUIRE_EQUAL(writer->entries.front()->message(), "one"_el);
        REQUIRE_FALSE(writer->lines.front()->text().find("one"_el).isNoIndex());
    }

    void testMessageByteLimitMarksTruncation() {
        auto options = el::log::LogManagerOptions{};
        options.setMaximumMessageBytes(el::unit::ByteLength{8U});
        const auto manager = el::log::LogManager::create(options);
        const auto writer = std::make_shared<CaptureWriter>();
        auto configuration = el::log::LogConfiguration{};
        configuration.setManagerOptions(options).addWriter(writer);
        manager->setConfiguration(std::move(configuration));
        manager->rootStream()->info("abcdefghijk"_el);
        manager->shutdown();

        REQUIRE_EQUAL(writer->entries.size(), std::size_t{1U});
        REQUIRE(writer->entries.front()->isTruncated());
        REQUIRE_LESS_EQUAL(writer->entries.front()->message().length(), el::unit::ByteLength{8U});
    }

    void testMessageByteLimitPreservesUtf8Boundary() {
        auto options = el::log::LogManagerOptions{};
        options.setMaximumMessageBytes(el::unit::ByteLength{5U});
        const auto manager = el::log::LogManager::create(options);
        const auto writer = std::make_shared<CaptureWriter>();
        auto configuration = el::log::LogConfiguration{};
        configuration.setManagerOptions(options).addWriter(writer);
        manager->setConfiguration(std::move(configuration));
        manager->rootStream()->info("aébcde"_el);
        manager->shutdown();

        REQUIRE_EQUAL(writer->entries.size(), std::size_t{1U});
        REQUIRE_EQUAL(writer->entries.front()->message(), "a…"_el);
        REQUIRE(writer->entries.front()->isTruncated());
    }

    void testWriterFailuresAreContainedAndCounted() {
        const auto manager = el::log::LogManager::create();
        const auto writer = std::make_shared<FailingWriter>();
        manager->setConfiguration(el::log::LogConfiguration{}.addWriter(writer));
        manager->rootStream()->error("failure"_el);
        manager->shutdown();

        REQUIRE_EQUAL(manager->statistics().writerFailures, uint64_t{1U});
    }

    void testWriterCannotBeActiveInTwoManagers() {
        const auto first = el::log::LogManager::create();
        const auto second = el::log::LogManager::create();
        const auto writer = std::make_shared<CaptureWriter>();
        first->setConfiguration(el::log::LogConfiguration{}.addWriter(writer));
        REQUIRE_THROWS(second->setConfiguration(el::log::LogConfiguration{}.addWriter(writer)));
        first->shutdown();
        second->shutdown();
    }

    void testInvalidManagerOptionsFailSynchronously() {
        auto options = el::log::LogManagerOptions{};
        REQUIRE_THROWS_AS(el::err::ParameterError, options.setMaximumEntries(0U));

        options = {};
        options.setReservedErrorBytes(options.maximumBytes() + el::unit::ByteLength::one());
        REQUIRE_THROWS_AS(el::err::ParameterError, el::log::LogConfiguration{}.setManagerOptions(options));
    }

    void testLineFormattingOptionsAndEscapedBraces() {
        const auto manager = el::log::LogManager::create();
        const auto writer = std::make_shared<CaptureWriter>();
        auto format = el::log::LogLineFormat{};
        format.setPattern("{{{level}}} {name}: {message}"_el)
            .setLevelFormat(el::log::LogLevelFormat::FullUpper)
            .setNameFormat(el::log::LogNameFormat::HeadAndLeaf)
            .setMessageTruncation(el::log::LogMessageTruncation::FirstLine)
            .setTruncationMark("[...]"_el);
        auto configuration = el::log::LogConfiguration{};
        configuration.setLineFormat(std::move(format)).addWriter(writer);
        manager->setConfiguration(std::move(configuration));
        manager->createStream("app/database/query"_el)->warn("first\nsecond"_el);
        manager->shutdown();

        REQUIRE_EQUAL(writer->lines.size(), std::size_t{1U});
        REQUIRE_EQUAL(writer->lines.front(), "{WARNING} app/…/query: first[...]"_el);
        REQUIRE_THROWS_AS(el::err::ParameterError, el::log::LogLineFormat{}.setPattern("{unknown}"_el));
    }

    void testLocalTimestampRenderingKeepsEntryTimestampUtc() {
        const auto manager = el::log::LogManager::create();
        const auto writer = std::make_shared<CaptureWriter>();
        auto format = el::log::LogLineFormat{};
        format.setPattern("{time}"_el).setTimestampZone(el::log::LogTimestampZone::Local);
        manager->setConfiguration(el::log::LogConfiguration{}.setLineFormat(format).addWriter(writer));
        manager->rootStream()->info("message"_el);
        manager->shutdown();

        REQUIRE_EQUAL(writer->entries.size(), std::size_t{1U});
        REQUIRE_EQUAL(writer->entries.front()->timestamp().timeOffset().toSeconds(), el::time::Seconds{});
        const auto expected = writer->entries.front()
                                  ->timestamp()
                                  .toTimeZone(el::time::TimeZone::local())
                                  .toIsoString(
                                      el::time::IsoTimeFormatFlags{
                                          el::time::IsoTimeFormat::Extended, el::time::IsoTimeFormat::TimeShift});
        REQUIRE_EQUAL(writer->lines.front(), expected);
    }
};
