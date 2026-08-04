// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ByteWriter.hpp"

#include "impl/ByteWriterTools.hpp"

namespace erbsland::mem {

using namespace unit;

void ByteWriter::setPosition(const ByteIndex position) noexcept {
    if (position.isNoIndex() || position > ByteIndex::end(length())) {
        _position = ByteIndex::end(length());
        return;
    }
    _position = position;
}

void ByteWriter::reset() noexcept {
    _block.reset();
    _position = ByteIndex::zero();
}

auto ByteWriter::takeByteBlockEditor() noexcept -> ByteBlockEditor {
    auto result = std::move(_block);
    _position = ByteIndex::zero();
    return result;
}

auto ByteWriter::reserve(const ByteLength capacity) -> ByteWriter & {
    _block.reserve(capacity);
    return *this;
}

auto ByteWriter::writeByte(const Byte value) -> ByteWriter & {
    if (_position >= ByteIndex::end(length())) {
        _block.append(value);
    } else {
        _block.set(_position, value);
    }
    setPosition(_position + ByteLength::one());
    return *this;
}

auto ByteWriter::writeBytes(const ConstByteSpan &data) -> ByteWriter & {
    if (_position >= ByteIndex::end(length())) {
        _block.append(data);
    } else {
        _block.overwrite(_position, data);
        if (_position + ByteLength::fromSizeT(data.size()) > ByteIndex::end(length())) {
            _block.append(data.subspan(length().toSizeT() - _position.toSizeT()));
        }
    }
    setPosition(_position + ByteLength::fromSizeT(data.size()));
    return *this;
}

auto ByteWriter::writeBytes(const std::span<const std::byte> &data) -> ByteWriter & {
    return writeBytes(std::span(reinterpret_cast<const Byte *>(data.data()), data.size()));
}

auto ByteWriter::writeBytes(const ByteBlock &data) -> ByteWriter & {
    return writeBytes(data.span());
}

auto ByteWriter::writeText(const text::String &text, const ByteTextOptions &options) -> ByteWriter & {
    impl::ByteWriterTools{*this}.writeText(text, options, false);
    return *this;
}

auto ByteWriter::writeTextOrThrow(const text::String &text, const ByteTextOptions &options) -> ByteWriter & {
    impl::ByteWriterTools{*this}.writeText(text, options, true);
    return *this;
}

}
