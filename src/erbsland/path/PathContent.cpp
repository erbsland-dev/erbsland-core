// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PathContent.hpp"

#include "Path.hpp"
#include "PathError.hpp"
#include "PathInfo.hpp"
#include "PathInfoParts.hpp"

#include "impl/BackendFactory.hpp"
#include "impl/PathBackend.hpp"
#include "impl/PathContent.hpp"

#include "../err/Exception.hpp"
#include "../err/OutOfRangeError.hpp"
#include "../mem/ByteArray.hpp"
#include "../mem/ByteBlock.hpp"
#include "../mem/impl/UnsafeByteBlockBuffer.hpp"
#include "../stream/ByteInputStream.hpp"
#include "../stream/ByteOutputStream.hpp"
#include "../stream/TextOutputStream.hpp"
#include "../text/Literals.hpp"
#include "../text/StringDecoder.hpp"
#include "../unit/ByteLength.hpp"

#include <algorithm>
#include <array>
#include <memory>
#include <utility>

namespace erbsland::path {

using namespace text::literals;
using namespace stream;
using namespace text;
using namespace unit;

PathContent::PathContent() = default;

PathContent::~PathContent() = default;

PathContent::PathContent(PathContent &&) noexcept = default;

auto PathContent::operator=(PathContent &&) noexcept -> PathContent & = default;

PathContent::PathContent(const Path &path) {
    if (!path.isEmpty()) {
        _impl = std::make_unique<impl::PathContent>(path);
    }
}

auto PathContent::isEmpty() const -> bool {
    return _impl == nullptr;
}

auto PathContent::path() const -> const Path & {
    return _impl == nullptr ? Path::empty() : _impl->path();
}

auto PathContent::readText(const PathReadTextOptions options) const noexcept -> std::optional<String> {
    try {
        return readTextOrThrow(options);
    } catch (const err::Exception &) {
        return std::nullopt;
    }
}

auto PathContent::readTextOrThrow(const PathReadTextOptions options) const -> String {
    const auto data = readDataOrThrow(
        PathReadDataOptions{}
            .setMaximumByteLength(options.maximumByteLength())
            .setStreamSettings(options.streamSettings()));
    auto result = StringDecoder{data}.decode(options.encoding(), options.bomMode(), options.encodingMode());
    if (options.maximumCpLength().isFinite() && result.characterLength() > options.maximumCpLength()) {
        throwError("Failed to read file data."_el, "The reported file size exceeds the maximum code-point length."_el);
    }
    return result;
}

auto PathContent::readData(const PathReadDataOptions options) const noexcept -> std::optional<mem::ByteBlock> {
    try {
        return readDataOrThrow(options);
    } catch (const err::Exception &) {
        return std::nullopt;
    }
}

auto PathContent::readDataOrThrow(const PathReadDataOptions options) const -> mem::ByteBlock {
    constexpr auto cBufferSize = ByteLength{16U * 1024U};
    if (isEmpty()) {
        throwPathEmptyError();
    }
    const auto maximumLength = options.maximumByteLength();
    if (maximumLength.isZero()) {
        return mem::ByteBlock{};
    }
    const auto info = path().info(PathInfoParts{PathInfoPart::Type, PathInfoPart::Size});
    auto initialCapacity = cBufferSize;
    auto knownFileSize = ByteLength::infinite();
    if (info.isRegularFile()) {
        // Add one byte as probe if the file size has changed after our probe.
        // This last byte is important! It is an "automatic" probe to detect if there is more data
        // after the reported file size.
        knownFileSize = info.fileSize();
        initialCapacity = knownFileSize + ByteLength::one();
    }
    if (maximumLength.isFinite()) {
        if (knownFileSize.isFinite() && knownFileSize > maximumLength) {
            throwError("Failed to read file data."_el, "The reported file size exceeds the maximum byte length."_el);
        }
        // Reserve one byte more than the maximum length to probe if we exceed it.
        initialCapacity = std::min(initialCapacity, maximumLength + ByteLength::one());
    }
    auto stream = openByteInputStream(options);
    auto buffer = mem::impl::UnsafeByteBlockBuffer{initialCapacity};
    auto totalLength = ByteLength::zero();
    while (true) {
        auto readSpan = buffer.remainingData(totalLength);
        assert(!readSpan.empty());
        const auto result = stream->read(readSpan);
        if (result == StreamReadStatus::Finished) {
            break;
        }
        if (result == StreamReadStatus::Timeout) {
            throwError(
                "Failed to read file data."_el,
                "The file input stream timed out before reaching the end of the file."_el);
        }
        if (result.data().isZero()) {
            throwError(
                "Failed to read file data."_el, "The file input stream reported data without providing any bytes."_el);
        }
        totalLength += result.data(); // increment the total length
        if (maximumLength.isFinite() && totalLength > maximumLength) {
            throwError("Failed to read file data."_el, "File data exceeds maximum byte length."_el);
        }
        if (totalLength == buffer.capacity()) {
            // if we reached the capacity, we need to make room for more data.
            auto requestedCapacity = totalLength + cBufferSize;
            if (maximumLength.isFinite()) {
                // if we have a maximum length, only request one byte more to probe if we exceed it.
                requestedCapacity = std::min(requestedCapacity, maximumLength + ByteLength::one());
            }
            assert(requestedCapacity > buffer.capacity());
            buffer.grow(requestedCapacity, totalLength);
        }
    }
    return buffer.take(totalLength);
}

auto PathContent::writeText(const String &text, const PathWriteTextOptions options) const noexcept -> util::Result {
    try {
        writeTextOrThrow(text, options);
        return util::Result::Success;
    } catch (const err::Exception &) {
        return util::Result::Failure;
    }
}

void PathContent::writeTextOrThrow(const String &text, const PathWriteTextOptions options) const {
    auto stream = openTextOutputStream(options);
    if (stream->write(text) == StreamWriteStatus::Timeout || stream->flush() == StreamWriteStatus::Timeout ||
        stream->close() == StreamCloseStatus::Timeout) {
        throwError(
            "Failed to write file text."_el, "The file output stream timed out before the text was fully written."_el);
    }
}

auto PathContent::writeData(const mem::ByteBlock &data, const PathWriteDataOptions options) const noexcept
    -> util::Result {
    try {
        writeDataOrThrow(data, options);
        return util::Result::Success;
    } catch (const err::Exception &) {
        return util::Result::Failure;
    }
}

void PathContent::writeDataOrThrow(const mem::ByteBlock &data, const PathWriteDataOptions options) const {
    auto stream = openByteOutputStream(options);
    if (stream->write(data) == StreamWriteStatus::Timeout || stream->flush() == StreamWriteStatus::Timeout ||
        stream->close() == StreamCloseStatus::Timeout) {
        throwError(
            "Failed to write file data."_el, "The file output stream timed out before the data was fully written."_el);
    }
}

auto PathContent::openTextInputStream(const PathReadTextOptions options) const -> TextInputStreamPtr {
    if (isEmpty()) {
        throwPathEmptyError();
    }
    return impl::pathBackend().openTextInputStreamOrThrow(path(), options);
}

auto PathContent::openTextOutputStream(const PathWriteTextOptions options) const -> TextOutputStreamPtr {
    if (isEmpty()) {
        throwPathEmptyError();
    }
    return impl::pathBackend().openTextOutputStreamOrThrow(path(), options);
}

auto PathContent::openByteInputStream(const PathReadDataOptions options) const -> ByteInputStreamPtr {
    if (isEmpty()) {
        throwPathEmptyError();
    }
    return impl::pathBackend().openByteInputStreamOrThrow(path(), options);
}

auto PathContent::openByteOutputStream(const PathWriteDataOptions options) const -> ByteOutputStreamPtr {
    if (isEmpty()) {
        throwPathEmptyError();
    }
    return impl::pathBackend().openByteOutputStreamOrThrow(path(), options);
}

void PathContent::throwError(String title, String description) const {
    throw PathError{PathErrorContext{std::move(title), std::move(description)}.setSourcePath(path().toString())};
}

void PathContent::throwPathEmptyError() const {
    throwError("File could not be opened"_el, "No path was provided for the operation."_el);
}

}
