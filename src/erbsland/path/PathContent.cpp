// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PathContent.hpp"

#include "Path.hpp"
#include "PathError.hpp"
#include "PathInfo.hpp"
#include "PathInfoParts.hpp"

#include "impl/BackendFactory.hpp"
#include "impl/PathContent.hpp"

#include "../err/Exception.hpp"
#include "../err/OutOfRangeError.hpp"
#include "../mem/ByteBlock.hpp"
#include "../mem/ByteBlockEditor.hpp"
#include "../stream/ByteInputStream.hpp"
#include "../stream/ByteOutputStream.hpp"
#include "../stream/TextOutputStream.hpp"
#include "../text/Literals.hpp"
#include "../text/StringDecoder.hpp"
#include "../unit/ByteLength.hpp"

#include <array>
#include <memory>

namespace erbsland::path {

using namespace text::literals;
using namespace stream;

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

auto PathContent::readText(const PathReadTextOptions options) const noexcept -> std::optional<text::String> {
    try {
        return readTextOrThrow(options);
    } catch (const err::Exception &) {
        return std::nullopt;
    }
}

auto PathContent::readTextOrThrow(const PathReadTextOptions options) const -> text::String {
    const auto data = readDataOrThrow(
        PathReadDataOptions{}
            .setMaximumByteLength(options.maximumByteLength())
            .setStreamSettings(options.streamSettings()));
    auto result = text::StringDecoder{data}.decode(options.encoding(), options.bomMode(), options.encodingErrorMode());
    if (options.maximumCpLength().isFinite() && result.characterLength() > options.maximumCpLength()) {
        throw err::OutOfRangeError{"File text exceeds maximum code-point length"_el};
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
    constexpr auto cBufferSize = std::size_t{16U * 1024U};

    if (isEmpty()) {
        throw PathError{
            PathErrorContext{"File could not be read"_el, "No path was provided for the read operation."_el}.setHelp(
                "Provide a non-empty path to the file."_el)};
    }

    if (options.maximumByteLength().isFinite()) {
        const auto info = path().info(PathInfoParts{PathInfoPart::Type, PathInfoPart::Size});
        if (info.isRegularFile() && info.fileSize() > options.maximumByteLength()) {
            throw err::OutOfRangeError{"File data exceeds maximum byte length"_el};
        }
    }

    // FIXME! This is inefficient, use `UnsafeByteBlockBuffer`,
    // If the file size could be determined, reserve the final size.
    // If not, start conservatively, e.g. 1kb, capped by maximumSize
    // and use `BestGrowth.hpp` to increase size in defined steps.
    // document this behaviour, so the user is aware that when reading really small
    // files, they have to set a small maximum size.
    auto stream = openByteInputStream(options);
    auto result = mem::ByteBlockEditor{};
    auto totalLength = unit::ByteLength::zero();
    auto buffer = std::array<mem::Byte, cBufferSize>{};
    while (true) {
        const auto readResult = stream->read(std::span<mem::Byte>{buffer});
        if (readResult == StreamReadStatus::Finished) {
            break;
        }
        if (readResult == StreamReadStatus::Timeout) {
            throw StreamError{StreamErrorContext{
                "Failed to read file data."_el,
                "The file input stream timed out before reaching the end of the file."_el}
                    .setPath(path().toString())};
        }
        const auto readLength = readResult.data();
        if (options.maximumByteLength().isFinite() && readLength > options.maximumByteLength() - totalLength) {
            throw err::OutOfRangeError{"File data exceeds maximum byte length"_el};
        }
        const auto block = mem::ByteBlock{std::span<const mem::Byte>{buffer.data(), readLength.toSizeT()}};
        result.append(block);
        totalLength += readLength;
    }
    return result;
}

auto PathContent::writeText(const text::String &text, const PathWriteTextOptions options) const noexcept
    -> util::Result {
    try {
        writeTextOrThrow(text, options);
        return util::Result::Success;
    } catch (const err::Exception &) {
        return util::Result::Failure;
    }
}

void PathContent::writeTextOrThrow(const text::String &text, const PathWriteTextOptions options) const {
    auto stream = openTextOutputStream(options);
    if (stream->write(text) == StreamWriteStatus::Timeout || stream->flush() == StreamWriteStatus::Timeout ||
        stream->close() == StreamCloseStatus::Timeout) {
        throw StreamError{StreamErrorContext{
            "Failed to write file text."_el, "The file output stream timed out before the text was fully written."_el}
                .setPath(path().toString())};
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
        throw StreamError{StreamErrorContext{
            "Failed to write file data."_el, "The file output stream timed out before the data was fully written."_el}
                .setPath(path().toString())};
    }
}

auto PathContent::openTextInputStream(const PathReadTextOptions options) const -> TextInputStreamPtr {
    if (isEmpty()) {
        throw PathError{PathErrorContext{
            "File could not be opened for reading"_el, "No path was provided for the read operation."_el}
                .setHelp("Provide a non-empty path to the file."_el)};
    }
    return impl::pathBackend().openTextInputStreamOrThrow(path(), options);
}

auto PathContent::openTextOutputStream(const PathWriteTextOptions options) const -> TextOutputStreamPtr {
    if (isEmpty()) {
        throw PathError{PathErrorContext{
            "File could not be opened for writing"_el, "No path was provided for the write operation."_el}
                .setHelp("Provide a non-empty path to the file."_el)};
    }
    return impl::pathBackend().openTextOutputStreamOrThrow(path(), options);
}

auto PathContent::openByteInputStream(const PathReadDataOptions options) const -> ByteInputStreamPtr {
    if (isEmpty()) {
        throw PathError{PathErrorContext{
            "File could not be opened for reading"_el, "No path was provided for the read operation."_el}
                .setHelp("Provide a non-empty path to the file."_el)};
    }
    return impl::pathBackend().openByteInputStreamOrThrow(path(), options);
}

auto PathContent::openByteOutputStream(const PathWriteDataOptions options) const -> ByteOutputStreamPtr {
    if (isEmpty()) {
        throw PathError{PathErrorContext{
            "File could not be opened for writing"_el, "No path was provided for the write operation."_el}
                .setHelp("Provide a non-empty path to the file."_el)};
    }
    return impl::pathBackend().openByteOutputStreamOrThrow(path(), options);
}

}
