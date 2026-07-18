// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TempTextOutputStream.hpp"

#include "StreamError.hpp"

#include "impl/IoService.hpp"

#include "../path/PathError.hpp"
#include "../path/PathOperations.hpp"
#include "../text/Literals.hpp"

#include <exception>
#include <utility>

namespace erbsland::stream {

using namespace text::literals;

TempTextOutputStream::TempTextOutputStream(path::Path path, TextOutputStreamPtr stream, const bool removeOnClose) :
    _path{std::move(path)},
    _stream{std::move(stream)},
    _settings{_stream->outputSettings()},
    _removeOnClose{removeOnClose} {
}

TempTextOutputStream::~TempTextOutputStream() {
    abort();
}

auto TempTextOutputStream::isEmpty() const noexcept -> bool {
    return _path.isEmpty();
}

auto TempTextOutputStream::path() const noexcept -> const path::Path & {
    return _path;
}

auto TempTextOutputStream::removeOnClose() const noexcept -> bool {
    return _removeOnClose;
}

void TempTextOutputStream::setRemoveOnClose(const bool value) noexcept {
    _removeOnClose = value;
}

auto TempTextOutputStream::release() noexcept -> path::Path {
    auto result = _path;
    _path = {};
    _removeOnClose = false;
    return result;
}

auto TempTextOutputStream::outputSettings() const noexcept -> const OutputStreamSettings & {
    return _settings;
}

auto TempTextOutputStream::state() const noexcept -> StreamState {
    return _stream != nullptr ? _stream->state() : StreamState::Closed;
}

auto TempTextOutputStream::isReady() const noexcept -> bool {
    return _stream != nullptr && _stream->isReady();
}

auto TempTextOutputStream::waitForReady() -> StreamWaitStatus {
    return _stream != nullptr ? _stream->waitForReady() : StreamWaitStatus::Timeout;
}

auto TempTextOutputStream::createErrorContext() const noexcept -> StreamErrorContext {
    auto context = _stream != nullptr ? _stream->createErrorContext() : StreamErrorSource::createErrorContext();
    if (!_path.isEmpty()) {
        context.setPath(_path.toString());
    }
    return context;
}

void TempTextOutputStream::throwError(
    text::String title, text::String description, system::PlatformErrorContextConstPtr platformContext) const {
    auto context = createErrorContext();
    context.setTitle(std::move(title))
        .setDescription(std::move(description))
        .setPlatformContext(std::move(platformContext));
    throw StreamError{std::move(context)};
}

auto TempTextOutputStream::flush() -> StreamWriteStatus {
    if (_stream == nullptr) {
        throwError(
            "Failed to flush the temporary output stream."_el, "The temporary text output stream is not open."_el);
    }
    return _stream->flush();
}

auto TempTextOutputStream::close() -> StreamCloseStatus {
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
        } catch (const path::PathError &error) {
            throwError(
                "Failed to remove the temporary file."_el,
                "The temporary output file could not be removed after closing the stream."_el,
                error.platformContext());
        }
    }
    return StreamCloseStatus::Closed;
}

void TempTextOutputStream::abort() noexcept {
    if (_stream != nullptr) {
        _stream->abort();
        _stream.reset();
    }
    if (!_path.isEmpty() && _removeOnClose) {
        auto path = _path;
        _path = {};
        _removeOnClose = false;
        impl::IoService::submitIoWork([path = std::move(path)]() -> void {
            try {
                path.operations().removeOrThrow();
            } catch (...) {}
        });
    }
}

auto TempTextOutputStream::encoding() const noexcept -> text::StringEncoding {
    return _stream != nullptr ? _stream->encoding() : text::StringEncoding::Utf8;
}

auto TempTextOutputStream::effectiveEncoding() const noexcept -> text::StringEncoding {
    return _stream != nullptr ? _stream->effectiveEncoding() : text::StringEncoding::Utf8;
}

auto TempTextOutputStream::write(const text::Char character) -> StreamWriteStatus {
    if (_stream == nullptr) {
        throwError(
            "Failed to write to the temporary output stream."_el, "The temporary text output stream is not open."_el);
    }
    return _stream->write(character);
}

auto TempTextOutputStream::write(const text::String &text) -> StreamWriteStatus {
    if (_stream == nullptr) {
        throwError(
            "Failed to write to the temporary output stream."_el, "The temporary text output stream is not open."_el);
    }
    return _stream->write(text);
}

auto TempTextOutputStream::writeLine() -> StreamWriteStatus {
    if (_stream == nullptr) {
        throwError(
            "Failed to write to the temporary output stream."_el, "The temporary text output stream is not open."_el);
    }
    return _stream->writeLine();
}

auto TempTextOutputStream::writeLine(const text::String &text) -> StreamWriteStatus {
    if (_stream == nullptr) {
        throwError(
            "Failed to write to the temporary output stream."_el, "The temporary text output stream is not open."_el);
    }
    return _stream->writeLine(text);
}

}
