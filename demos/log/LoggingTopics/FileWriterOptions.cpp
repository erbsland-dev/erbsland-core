// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/log/all.hpp>
#include <erbsland/log/line/all.hpp>

#include <memory>
#include <utility>
#include <vector>

namespace demo {

auto createFileWriterDemoDirectory() -> el::TempDirectoryPtr {
    auto options = el::PathTempDirectoryOptions{};
    options.setPrefix("retkikunta-"_el).setRandomLength(el::CpLength{8U});
    return el::Path::systemTempDirectoryOrThrow().operations().createTempDirectoryOrThrow(options);
}

void writeFileDemoLine(
    const el::LogWriterPtr &writer,
    const el::DateTime &timestamp,
    const el::String &message,
    const uint64_t sequence = 1U) {
    const auto entry = std::make_shared<el::LogEntry>(
        sequence, timestamp, el::LogLevel::Information, el::LogPath{"guild/journal"_el}, message);
    const auto line =
        std::make_shared<el::LogLine>(std::vector<el::LogLineSegment>{{el::LogLinePart::Message, message}});
    writer->write(entry, line);
}

/// The initial file mode decides whether the first successful open preserves or replaces existing content.
///
/// Append is the default. Overwrite affects only the first open of one writer; recovery after an interruption always
/// appends so data written since application startup is not erased unexpectedly.
void fileWriterModes() {
    const auto temporary = createFileWriterDemoDirectory();
    const auto appendPath = temporary->path() / "append.log"_el;
    const auto overwritePath = temporary->path() / "overwrite.log"_el;
    appendPath.content().writeTextOrThrow("Earlier run\n"_el);
    overwritePath.content().writeTextOrThrow("Earlier run\n"_el);

    const auto appendWriter = el::LogWriter::createForFile(el::FileLogWriterOptions{appendPath});
    writeFileDemoLine(appendWriter, el::DateTime::now(), "Current run"_el);
    appendWriter->close();

    auto overwriteOptions = el::FileLogWriterOptions{overwritePath};
    overwriteOptions.setMode(el::LogFileMode::Overwrite);
    const auto overwriteWriter = el::LogWriter::createForFile(overwriteOptions);
    writeFileDemoLine(overwriteWriter, el::DateTime::now(), "Current run"_el);
    overwriteWriter->close();

    const auto yesNo = el::BooleanFormat::yesNo();
    el::io::printLine(
        "Append preserved earlier content   : "_el,
        yesNo,
        appendPath.content().readTextOrThrow().contains("Earlier run"_el));
    el::io::printLine(
        "Overwrite preserved earlier content: "_el,
        yesNo,
        overwritePath.content().readTextOrThrow().contains("Earlier run"_el));
}

/// A rotation policy selects when the active file becomes an archive.
///
/// Hourly, daily, and weekly modes compare UTC entry timestamps. This deterministic daily example moves the first
/// day's file to a date-keyed archive before it writes the first entry from the next day.
void fileWriterRotation() {
    const auto temporary = createFileWriterDemoDirectory();
    const auto path = temporary->path() / "guild.log"_el;
    auto options = el::FileLogWriterOptions{path};
    options.setMode(el::LogFileMode::Overwrite).setRotation(el::LogFileRotation::Daily);
    const auto writer = el::LogWriter::createForFile(options);

    writeFileDemoLine(
        writer,
        el::DateTime{el::Date::fromYearMonthDay(2026, 9, 1), el::Time{el::Hour{18}, el::Minute{0}}},
        "First day"_el,
        1U);
    writeFileDemoLine(
        writer,
        el::DateTime{el::Date::fromYearMonthDay(2026, 9, 2), el::Time{el::Hour{7}, el::Minute{0}}},
        "Second day"_el,
        2U);
    writer->close();

    const auto archive = path.withStem("guild.2026-09-01"_el);
    el::io::printLine("Daily archive: "_el, archive.name());
    el::io::printLine("Archive exists: "_el, el::BooleanFormat::yesNo(), archive.info().exists());
}

/// The maximum-size option supplies the byte threshold used by size rotation.
///
/// The writer counts the encoded line and its line break. It rotates a nonempty active file before the next write would
/// cross the threshold, then writes that complete line to a fresh active file.
void fileWriterMaximumSize() {
    const auto temporary = createFileWriterDemoDirectory();
    const auto path = temporary->path() / "guild.log"_el;
    auto options = el::FileLogWriterOptions{path};
    options.setMode(el::LogFileMode::Overwrite)
        .setRotation(el::LogFileRotation::Size)
        .setMaximumSize(el::ByteLength{24U});
    const auto writer = el::LogWriter::createForFile(options);

    writeFileDemoLine(writer, el::DateTime::now(), "First observation"_el, 1U);
    writeFileDemoLine(writer, el::DateTime::now(), "Second observation"_el, 2U);
    writer->close();

    el::io::printLine(
        "Rotation created guild.1.log: "_el, el::BooleanFormat::yesNo(), path.withStem("guild.1"_el).info().exists());
}

/// Retention bounds the number of archives managed by the writer.
///
/// Size rotation discovers and shifts numbered archives at the destination. A retention of two keeps `.1` and `.2`;
/// older numbered history is removed as later rotations occur.
void fileWriterRetention() {
    const auto temporary = createFileWriterDemoDirectory();
    const auto path = temporary->path() / "guild.log"_el;
    auto options = el::FileLogWriterOptions{path};
    options.setMode(el::LogFileMode::Overwrite)
        .setRotation(el::LogFileRotation::Size)
        .setMaximumSize(el::ByteLength{12U})
        .setRetention(2U);
    const auto writer = el::LogWriter::createForFile(options);

    for (auto sequence = uint64_t{1U}; sequence <= 4U; ++sequence) {
        writeFileDemoLine(writer, el::DateTime::now(), "Entry 0001"_el, sequence);
    }
    writer->close();

    const auto yesNo = el::BooleanFormat::yesNo();
    el::io::printLine("guild.1.log exists: "_el, yesNo, path.withStem("guild.1"_el).info().exists());
    el::io::printLine("guild.2.log exists: "_el, yesNo, path.withStem("guild.2"_el).info().exists());
    el::io::printLine("guild.3.log exists: "_el, yesNo, path.withStem("guild.3"_el).info().exists());
}

}
