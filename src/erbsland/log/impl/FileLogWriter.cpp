// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "FileLogWriter.hpp"

#include "../line/LogLine.hpp"

#include "../../err/ParameterError.hpp"
#include "../../err/RuntimeError.hpp"
#include "../../path/PathCollisionMode.hpp"
#include "../../path/PathContent.hpp"
#include "../../path/PathCreateMode.hpp"
#include "../../path/PathInfo.hpp"
#include "../../path/PathMoveOptions.hpp"
#include "../../path/PathOperations.hpp"
#include "../../path/PathWriteTextOptions.hpp"
#include "../../stream/TextOutputStream.hpp"
#include "../../text/Literals.hpp"
#include "../../text/StringEditor.hpp"
#include "../../text/StringSide.hpp"
#include "../../time/DayOfWeek.hpp"

#include <algorithm>
#include <utility>

namespace erbsland::log::impl {

using namespace text::literals;
using namespace unit;

FileLogWriter::FileLogWriter(FileLogWriterOptions options) : _options{std::move(options)} {
    if (_options.path().isEmpty()) {
        throw err::ParameterError{"A file log writer requires a path."_el, "path"_el};
    }
    if (_options.rotation() == LogFileRotation::Size && _options.maximumSize().isZero()) {
        throw err::ParameterError{"Size rotation requires a nonzero maximum size."_el, "maximumSize"_el};
    }
}

void FileLogWriter::write(const LogEntryConstPtr &entry, const LogLineConstPtr &line) {
    const auto item = BatchItem{entry, line};
    writeBatch(Batch{&item, 1U});
}

void FileLogWriter::writeBatch(const Batch batch) {
    if (batch.empty()) {
        return;
    }
    try {
        ensureOpen();
        verifyIdentity();
        ensureOpen();
        for (const auto &item : batch) {
            const auto &entry = item.entry();
            const auto &line = item.line();
            if (_rotationKey.isEmpty() &&
                (_options.rotation() == LogFileRotation::Hourly || _options.rotation() == LogFileRotation::Daily ||
                    _options.rotation() == LogFileRotation::Weekly)) {
                _rotationKey = rotationKey(*entry);
            }
            const auto writeSize = line->text().length() + ByteLength::one();
            if (shouldRotate(*entry, writeSize)) {
                rotate(*entry);
                ensureOpen();
            }
            if (_stream->writeLine(line->text()) != stream::StreamWriteStatus::Success) {
                throw err::RuntimeError{"Timed out while writing a log file."_el};
            }
            _currentSize += writeSize;
        }
        _retryStep = 0U;
        _nextRetry = {};
    } catch (...) {
        closeStream();
        scheduleRetry();
        throw;
    }
}

void FileLogWriter::flush() {
    try {
        if (_stream && _stream->flush() != stream::StreamWriteStatus::Success) {
            throw err::RuntimeError{"Timed out while flushing a log file."_el};
        }
    } catch (...) {
        closeStream();
        scheduleRetry();
        throw;
    }
}

void FileLogWriter::close() noexcept {
    closeStream();
}

void FileLogWriter::ensureOpen() {
    if (_stream) {
        return;
    }
    if (_nextRetry != time::TimePoint{} && time::TimePoint::now() < _nextRetry) {
        throw err::RuntimeError{"The log file is waiting for its next recovery attempt."_el};
    }
    auto options = path::PathWriteTextOptions{};
    options.setCreateParents(true);
    const auto append = _openedOnce || _options.mode() == LogFileMode::Append;
    options.setCreationMode(append ? path::PathCreateMode::CreateOrAppend : path::PathCreateMode::CreateOrOverwrite);
    _stream = _options.path().content().openTextOutputStream(options);
    _identity = _stream->fileIdentity();
    auto info = _options.path().info(path::PathInfoParts{path::PathInfoPart::Size, path::PathInfoPart::FileIdentity});
    _currentSize = info.fileSize();
    _openedOnce = true;
    _nextIdentityCheck = time::TimePoint::inFuture(time::TimeDelta::seconds(1));
}

void FileLogWriter::verifyIdentity() {
    if (!_stream || time::TimePoint::now() < _nextIdentityCheck) {
        return;
    }
    auto info = _options.path().info(path::PathInfoPart::FileIdentity);
    info.reload(path::PathInfoPart::FileIdentity);
    _nextIdentityCheck = time::TimePoint::inFuture(time::TimeDelta::seconds(1));
    if (!info.fileIdentity().isValid() || info.fileIdentity() != _identity) {
        closeStream();
    }
}

auto FileLogWriter::shouldRotate(const LogEntry &entry, const ByteLength writeSize) const -> bool {
    switch (_options.rotation().toRawValue()) {
    case LogFileRotation::None:
        return false;
    case LogFileRotation::Size:
        return !_currentSize.isZero() &&
            writeSize > _options.maximumSize() - std::min(_currentSize, _options.maximumSize());
    case LogFileRotation::Hourly:
    case LogFileRotation::Daily:
    case LogFileRotation::Weekly:
        return !_rotationKey.isEmpty() && rotationKey(entry) != _rotationKey;
    }
    return false;
}

void FileLogWriter::rotate(const LogEntry &entry) {
    if (_options.rotation() == LogFileRotation::Size) {
        rotateBySize();
    } else {
        rotateByTime();
    }
    _rotationKey = rotationKey(entry);
}

void FileLogWriter::rotateBySize() {
    closeStream();
    if (_options.retention() == 0U) {
        _options.path().operations().removeOrThrow();
        return;
    }
    auto moveOptions = path::PathMoveOptions{};
    moveOptions.setCollisionMode(path::PathCollisionMode::Overwrite);
    for (auto index = _options.retention(); index > 1U; --index) {
        const auto source = archivePath(text::String::fromInteger(index - 1U));
        if (source.info().exists()) {
            source.operations().moveToOrThrow(archivePath(text::String::fromInteger(index)), moveOptions);
        }
    }
    if (_options.path().info().exists()) {
        _options.path().operations().moveToOrThrow(archivePath("1"_el), moveOptions);
    }
}

void FileLogWriter::rotateByTime() {
    closeStream();
    if (_rotationKey.isEmpty() || !_options.path().info().exists()) {
        return;
    }
    const auto archive = archivePath(_rotationKey);
    auto options = path::PathMoveOptions{};
    options.setCollisionMode(path::PathCollisionMode::Overwrite);
    _options.path().operations().moveToOrThrow(archive, options);
    _archives.push_back(archive);
    while (_archives.size() > _options.retention()) {
        _archives.front().operations().remove();
        _archives.pop_front();
    }
}

auto FileLogWriter::archivePath(const text::String &key) const -> path::Path {
    return _options.path().withStem(text::String::fromJoined({_options.path().stem(), "."_el, key}));
}

auto FileLogWriter::rotationKey(const LogEntry &entry) const -> text::String {
    const auto date = entry.timestamp().date();
    switch (_options.rotation().toRawValue()) {
    case LogFileRotation::Hourly:
        return text::String::fromJoined(
            {date.toString(),
                "-"_el,
                entry.timestamp().time().toString().slice(text::StringSide::Front, unit::ByteLength{2U})});
    case LogFileRotation::Daily:
        return date.toString();
    case LogFileRotation::Weekly:
        return date.added(-date.dayOfWeek().toAmount()).toString();
    case LogFileRotation::None:
    case LogFileRotation::Size:
        return {};
    }
    return {};
}

void FileLogWriter::closeStream() noexcept {
    if (!_stream) {
        return;
    }
    if (_stream->close() != stream::StreamCloseStatus::Closed) {
        _stream->abort();
    }
    _stream.reset();
    _identity = {};
}

void FileLogWriter::scheduleRetry() noexcept {
    constexpr auto cMaximumStep = 8U;
    const auto now = time::TimePoint::now();
    if (_nextRetry != time::TimePoint{} && now < _nextRetry) {
        return;
    }
    const auto delay = 100 * (1 << std::min(_retryStep, cMaximumStep));
    _nextRetry = time::TimePoint::inFuture(time::TimeDelta::milliseconds(std::min(delay, 30'000)));
    _retryStep = std::min(_retryStep + 1U, cMaximumStep);
}

}
