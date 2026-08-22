// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ByteDecompressor.hpp"

#include "ByteCompressionError.hpp"

#include "impl/ByteCompressionCodec.hpp"

#include "../err/LogicError.hpp"
#include "../err/OutOfRangeError.hpp"
#include "../text/Literals.hpp"

#include <cstddef>

namespace erbsland::mem {

using namespace text::literals;

ByteDecompressor::ByteDecompressor(const ByteCompressionAlgorithm algorithm) : _algorithm{algorithm} {
    if (algorithm != ByteCompressionAlgorithm::Lz4Block) {
        throw ByteCompressionError{
            ByteCompressionErrorReason::UnsupportedAlgorithm, "The compression algorithm is unsupported."_el};
    }
}

auto ByteDecompressor::decompress(const ConstByteSpan data, const unit::ByteLength originalLength) const -> ByteBlock {
    if (!_algorithm.has_value()) {
        throw err::LogicError{"Raw decompression requires a configured algorithm."_el};
    }
    return impl::ByteCompressionCodec{*_algorithm}.decompress(data, originalLength, false);
}

auto ByteDecompressor::decompress(const ByteBlock &data, const unit::ByteLength originalLength) const -> ByteBlock {
    if (!_algorithm.has_value()) {
        throw err::LogicError{"Raw decompression requires a configured algorithm."_el};
    }
    return impl::ByteCompressionCodec{*_algorithm}.decompress(data.span(), originalLength, data.isSensitive());
}

auto ByteDecompressor::decompressWithEnvelope(const ConstByteSpan data, const unit::ByteLength maximumOutputSize)
    -> ByteBlock {
    return decompressEnvelope(data, maximumOutputSize, false);
}

auto ByteDecompressor::decompressWithEnvelope(const ByteBlock &data, const unit::ByteLength maximumOutputSize)
    -> ByteBlock {
    return decompressEnvelope(data.span(), maximumOutputSize, data.isSensitive());
}

void ByteDecompressor::update(const ConstByteSpan data) {
    requireNotFinalized();
    _input.append(data);
}

void ByteDecompressor::update(const ByteBlock &data) {
    requireNotFinalized();
    _input.append(data);
}

auto ByteDecompressor::finalize(const unit::ByteLength originalLength) -> ByteBlock {
    requireFinalizationMode(FinalizationMode::Raw);
    if (!_algorithm.has_value()) {
        throw err::LogicError{"Raw decompression requires a configured algorithm."_el};
    }
    if (_finalizationMode == FinalizationMode::None) {
        _result = decompress(ByteBlock{_input}, originalLength);
        _finalizationMode = FinalizationMode::Raw;
    }
    return _result;
}

auto ByteDecompressor::finalizeWithEnvelope(const unit::ByteLength maximumOutputSize) -> ByteBlock {
    requireFinalizationMode(FinalizationMode::Envelope);
    if (_finalizationMode == FinalizationMode::None) {
        _result = decompressWithEnvelope(ByteBlock{_input}, maximumOutputSize);
        _finalizationMode = FinalizationMode::Envelope;
    }
    return _result;
}

void ByteDecompressor::reset() noexcept {
    _input.reset();
    _result = {};
    _finalizationMode = FinalizationMode::None;
}

auto ByteDecompressor::decompressEnvelope(
    const ConstByteSpan data, const unit::ByteLength maximumOutputSize, const bool sensitive) -> ByteBlock {
    constexpr auto cHeaderSize = std::size_t{24U};
    if (data.size() < cHeaderSize) {
        throw ByteCompressionError{ByteCompressionErrorReason::MalformedData, "Compression envelope is truncated."_el};
    }
    if (data[0].toChar() != 'E' || data[1].toChar() != 'L' || data[2].toChar() != 'B' || data[3].toChar() != 'C') {
        throw ByteCompressionError{
            ByteCompressionErrorReason::MalformedData, "Compression envelope magic is invalid."_el};
    }
    if (data[4].toUInt8() != 1U) {
        throw ByteCompressionError{
            ByteCompressionErrorReason::UnsupportedEnvelopeVersion, "Compression envelope version is unsupported."_el};
    }
    if (data[6].toUInt8() != 0U || data[7].toUInt8() != 0U) {
        throw ByteCompressionError{
            ByteCompressionErrorReason::MalformedData, "Compression envelope flags are unsupported."_el};
    }
    const auto algorithmValue = data[5].toUInt8();
    if (algorithmValue != static_cast<uint8_t>(ByteCompressionAlgorithm::Lz4Block)) {
        throw ByteCompressionError{
            ByteCompressionErrorReason::UnsupportedAlgorithm, "Compression envelope algorithm is unsupported."_el};
    }
    const auto originalValue = readUInt64(data, 8U);
    const auto payloadValue = readUInt64(data, 16U);
    if (originalValue >= unit::ByteLength::cRawInfinite || payloadValue > data.size() - cHeaderSize ||
        payloadValue != data.size() - cHeaderSize) {
        throw ByteCompressionError{
            ByteCompressionErrorReason::LengthMismatch, "Compression envelope lengths are inconsistent."_el};
    }
    const auto originalLength = unit::ByteLength{originalValue};
    if (maximumOutputSize.isFinite() && originalLength > maximumOutputSize) {
        throw err::OutOfRangeError{"Compression envelope output exceeds the configured maximum."_el};
    }
    const auto algorithm = ByteCompressionAlgorithm{ByteCompressionAlgorithm::Lz4Block};
    return impl::ByteCompressionCodec{algorithm}.decompress(
        data.subspan(cHeaderSize, static_cast<std::size_t>(payloadValue)), originalLength, sensitive);
}

auto ByteDecompressor::readUInt64(const ConstByteSpan data, const std::size_t offset) noexcept -> uint64_t {
    auto result = uint64_t{};
    for (auto index = std::size_t{}; index < sizeof(uint64_t); ++index) {
        result |= data[offset + index].toUInt64() << (index * 8U);
    }
    return result;
}

void ByteDecompressor::requireNotFinalized() const {
    if (isFinalized()) {
        throw err::LogicError{"Cannot update a finalized byte decompressor."_el};
    }
}

void ByteDecompressor::requireFinalizationMode(const FinalizationMode mode) const {
    if (_finalizationMode != FinalizationMode::None && _finalizationMode != mode) {
        throw err::LogicError{"Byte decompressor was finalized using another input format."_el};
    }
}

}
