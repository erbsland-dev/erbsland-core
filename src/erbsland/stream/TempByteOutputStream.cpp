// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TempByteOutputStream.hpp"

#include "StreamError.hpp"

#include "../path/impl/FileCommit.hpp"
#include "../path/PathError.hpp"
#include "../path/PathOperations.hpp"
#include "../text/Literals.hpp"

#include <exception>
#include <utility>

namespace erbsland::stream {

using namespace path;
using namespace text::literals;

TempByteOutputStream::TempByteOutputStream(
    Path path, ByteOutputStreamPtr stream, const bool removeOnClose, util::impl::WorkerReservation cleanup) :
    _cleanup{std::move(cleanup)},
    _path{std::move(path)},
    _stream{std::move(stream)},
    _settings{_stream->outputSettings()},
    _removeOnClose{removeOnClose} {
}

TempByteOutputStream::~TempByteOutputStream() {
    abort();
}

auto TempByteOutputStream::isEmpty() const noexcept -> bool {
    return _path.isEmpty();
}

auto TempByteOutputStream::path() const noexcept -> const Path & {
    return _path;
}

auto TempByteOutputStream::removeOnClose() const noexcept -> bool {
    return _removeOnClose;
}

void TempByteOutputStream::setRemoveOnClose(const bool value) noexcept {
    _removeOnClose = value;
}

auto TempByteOutputStream::release() noexcept -> Path {
    auto result = _path;
    _path = {};
    _removeOnClose = false;
    return result;
}

auto TempByteOutputStream::outputSettings() const noexcept -> const OutputStreamSettings & {
    return _settings;
}

auto TempByteOutputStream::state() const noexcept -> StreamState {
    return _stream != nullptr ? _stream->state() : StreamState::Closed;
}

auto TempByteOutputStream::isReady() const noexcept -> bool {
    return _stream != nullptr && _stream->isReady();
}

auto TempByteOutputStream::waitForReady() -> StreamWaitStatus {
    return _stream != nullptr ? _stream->waitForReady() : StreamWaitStatus::Timeout;
}

auto TempByteOutputStream::createErrorContext() const noexcept -> StreamErrorContext {
    auto context = _stream != nullptr ? _stream->createErrorContext() : StreamErrorSource::createErrorContext();
    if (!_path.isEmpty()) {
        context.setPath(_path.toString());
    }
    return context;
}

void TempByteOutputStream::throwError(
    text::String title, text::String description, system::PlatformErrorContextConstPtr platformContext) const {
    auto context = createErrorContext();
    context.setTitle(std::move(title))
        .setDescription(std::move(description))
        .setPlatformContext(std::move(platformContext));
    throw StreamError{std::move(context)};
}

auto TempByteOutputStream::commitTo(const Path &destination, PathCommitOptions options) noexcept -> util::Result {
    try {
        commitToOrThrow(destination, std::move(options));
        return util::Result::Success;
    } catch (const PathError &) {
        return util::Result::Failure;
    }
}

void TempByteOutputStream::commitToOrThrow(const Path &destination, PathCommitOptions options) {
    auto operation = path::impl::FileCommit{_path, destination, std::move(options), _settings};
    operation.commit(_stream.get());
    _stream.reset();
    _path = {};
    _removeOnClose = false;
}

auto TempByteOutputStream::flush() -> StreamWriteStatus {
    if (_stream == nullptr) {
        throwError(
            "Failed to flush the temporary output stream."_el, "The temporary byte output stream is not open."_el);
    }
    return _stream->flush();
}

auto TempByteOutputStream::close() -> StreamCloseStatus {
    if (_stream != nullptr) {
        const auto result = _stream->close();
        if (result == StreamCloseStatus::Timeout) {
            return result;
        }
        _stream.reset();
    }
    if (!_path.isEmpty() && _removeOnClose) {
        try {
            _path.operations().removeOrThrow();
            _path = {};
            _removeOnClose = false;
        } catch (const PathError &error) {
            throwError(
                "Failed to remove the temporary file."_el,
                "The temporary output file could not be removed after closing the stream."_el,
                error.platformContext());
        }
    }
    return StreamCloseStatus::Closed;
}

void TempByteOutputStream::abort() noexcept {
    if (_stream != nullptr) {
        _stream->abort();
        _stream.reset();
    }
    if (!_path.isEmpty() && _removeOnClose) {
        auto path = _path;
        _path = {};
        _removeOnClose = false;
        _cleanup.submit([path = std::move(path)]() -> void {
            try {
                path.operations().removeOrThrow();
            } catch (const PathError &) {}
        });
    }
}

auto TempByteOutputStream::endianness() const noexcept -> mem::Endianness {
    return _stream != nullptr ? _stream->endianness() : ByteOutputStream::endianness();
}

void TempByteOutputStream::setEndianness(const mem::Endianness endianness) noexcept {
    if (_stream != nullptr) {
        _stream->setEndianness(endianness);
    }
    ByteOutputStream::setEndianness(endianness);
}

auto TempByteOutputStream::write(const mem::ConstByteSpan bytes) -> StreamWriteStatus {
    if (_stream == nullptr) {
        throwError(
            "Failed to write to the temporary output stream."_el, "The temporary byte output stream is not open."_el);
    }
    return _stream->write(bytes);
}

}
