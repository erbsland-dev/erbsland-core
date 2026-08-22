// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ByteCompressor.hpp"

#include "ByteCompressionError.hpp"

#include "impl/ByteCompressionCodec.hpp"

#include "../err/LogicError.hpp"
#include "../text/Literals.hpp"

namespace erbsland::mem {

using namespace text::literals;

ByteCompressor::ByteCompressor(const ByteCompressionAlgorithm algorithm) : _algorithm{algorithm} {
    if (algorithm != ByteCompressionAlgorithm::Lz4Block) {
        throw ByteCompressionError{
            ByteCompressionErrorReason::UnsupportedAlgorithm, "The compression algorithm is unsupported."_el};
    }
}

auto ByteCompressor::compress(const ConstByteSpan data) const -> ByteBlock {
    return impl::ByteCompressionCodec{_algorithm}.compress(data, false);
}

auto ByteCompressor::compress(const ByteBlock &data) const -> ByteBlock {
    return impl::ByteCompressionCodec{_algorithm}.compress(data.span(), data.isSensitive());
}

auto ByteCompressor::compressWithEnvelope(const ConstByteSpan data) const -> ByteBlock {
    return createEnvelope(compress(data), unit::ByteLength::fromSizeT(data.size()), false);
}

auto ByteCompressor::compressWithEnvelope(const ByteBlock &data) const -> ByteBlock {
    return createEnvelope(compress(data), data.length(), data.isSensitive());
}

void ByteCompressor::update(const ConstByteSpan data) {
    requireNotFinalized();
    _input.append(data);
}

void ByteCompressor::update(const ByteBlock &data) {
    requireNotFinalized();
    _input.append(data);
}

auto ByteCompressor::finalize() -> ByteBlock {
    requireFinalizationMode(FinalizationMode::Raw);
    if (_finalizationMode == FinalizationMode::None) {
        _result = compress(ByteBlock{_input});
        _finalizationMode = FinalizationMode::Raw;
    }
    return _result;
}

auto ByteCompressor::finalizeWithEnvelope() -> ByteBlock {
    requireFinalizationMode(FinalizationMode::Envelope);
    if (_finalizationMode == FinalizationMode::None) {
        _result = compressWithEnvelope(ByteBlock{_input});
        _finalizationMode = FinalizationMode::Envelope;
    }
    return _result;
}

void ByteCompressor::reset() noexcept {
    _input.reset();
    _result = {};
    _finalizationMode = FinalizationMode::None;
}

auto ByteCompressor::createEnvelope(
    const ByteBlock &compressed, const unit::ByteLength originalLength, const bool sensitive) const -> ByteBlock {
    auto envelope = ByteBlockEditor{};
    envelope.reserve(unit::ByteLength{24U}.addedOrThrow(compressed.length()));
    envelope.append(Byte::fromChar('E'));
    envelope.append(Byte::fromChar('L'));
    envelope.append(Byte::fromChar('B'));
    envelope.append(Byte::fromChar('C'));
    envelope.append(Byte{1U});
    envelope.append(Byte{static_cast<uint8_t>(_algorithm.toRawValue())});
    envelope.appendInteger<uint16_t>(0U);
    envelope.appendInteger<uint64_t>(originalLength.toRawValue());
    envelope.appendInteger<uint64_t>(compressed.length().toRawValue());
    envelope.append(compressed);
    if (sensitive) {
        envelope.markAsSensitive();
    }
    return ByteBlock{envelope};
}

void ByteCompressor::requireNotFinalized() const {
    if (isFinalized()) {
        throw err::LogicError{"Cannot update a finalized byte compressor."_el};
    }
}

void ByteCompressor::requireFinalizationMode(const FinalizationMode mode) const {
    if (_finalizationMode != FinalizationMode::None && _finalizationMode != mode) {
        throw err::LogicError{"Byte compressor was finalized using another output format."_el};
    }
}

}
