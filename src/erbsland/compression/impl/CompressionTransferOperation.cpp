// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "CompressionTransferOperation.hpp"

#include "../../err/ParameterError.hpp"
#include "../../mem/ByteWriter.hpp"
#include "../../text/Literals.hpp"

#include <algorithm>
#include <utility>

namespace erbsland::compression::impl {

using namespace text::literals;

CompressionTransferOperation::CompressionTransferOperation(
    const CompressionCodec &codec,
    const bool decompress,
    CodecReader::Read read,
    CodecOutput::Write write,
    const CompressionTransferOptions &transfer,
    const DecompressionOptions &decode,
    const CompressionOptions &encode) :
    _codec{codec},
    _decompress{decompress},
    _read{std::move(read)},
    _write{std::move(write)},
    _transfer{transfer},
    _decode{decode},
    _encode{encode},
    _coreEnvelope{codec.format() == CompressionFormat::Core},
    _source{[this] { return readInput(); }},
    _target{[this](mem::ConstByteSpan bytes) { writeOutput(bytes); }} {
}

auto CompressionTransferOperation::run() -> CompressionTransferResult {
    if (!_transfer.bufferLength().isFinite() || _transfer.bufferLength().toRawValue() == 0U ||
        !_transfer.maximumBufferedLength().isFinite() ||
        (_transfer.inputLength() && !_transfer.inputLength()->isFinite()) ||
        _transfer.fallbackPolicy() > CompressionFallbackPolicy::RequireStreaming) {
        throw err::ParameterError{"Invalid compression transfer limits."_el, "transfer"_el};
    }
    if (_transfer.fallbackPolicy() == CompressionFallbackPolicy::RequireStreaming && !_codec.supportsStreaming()) {
        throw CompressionError{
            CompressionErrorReason::UnsupportedFeature, "The selected codec requires whole-value buffering."_el};
    }
    _progress.inputTotal = _transfer.inputLength();
    _progress.outputTotal = _decompress ? _decode.expectedOutputLength() : std::nullopt;
    notify();
    if (_codec.format() != CompressionFormat::Core) {
        if (_decompress) {
            _codec.decompressPayload(_source, _target, _transfer, _decode);
        } else {
            _codec.compressPayload(_source, _target, _transfer, _encode);
        }
    } else if (!_decompress) {
        compressCore();
    } else {
        decompressCore();
    }
    _source.requireEnd();
    if (_transfer.inputLength() && _progress.transferred.inputLength != *_transfer.inputLength()) {
        throw CompressionError{CompressionErrorReason::LengthMismatch, "Input ended before the requested prefix."_el};
    }
    if (_decompress && _decode.expectedOutputLength() &&
        _progress.transferred.outputLength != *_decode.expectedOutputLength()) {
        throw CompressionError{CompressionErrorReason::LengthMismatch, "Decompressed length mismatch."_el};
    }
    _progress.phase = CompressionPhase::Finalizing;
    notify();
    _progress.phase = CompressionPhase::Completed;
    notify();
    return _progress.transferred;
}

void CompressionTransferOperation::notify() {
    if (_transfer.progress() && _transfer.progress()(_progress) == CompressionProgressAction::Cancel &&
        _progress.phase != CompressionPhase::Completed) {
        throw CompressionError{CompressionErrorReason::Cancelled, "Compression transfer was cancelled."_el};
    }
}

auto CompressionTransferOperation::readInput() -> mem::ByteBlock {
    auto data = _read();
    _progress.transferred.inputLength = _progress.transferred.inputLength.addedOrThrow(data.length());
    if (!_decompress && _coreEnvelope) {
        _crc.update(data.span());
    }
    notify();
    return data;
}

void CompressionTransferOperation::writeOutput(mem::ConstByteSpan bytes) {
    if (_decompress) {
        const auto next = unit::ByteLength{_uncompressed}.addedOrThrow(unit::ByteLength::fromSizeT(bytes.size()));
        if (next > _decode.maximumOutputLength()) {
            throw err::OutOfRangeError{"Decompressed output limit exceeded."_el};
        }
        _uncompressed = next.toRawValue();
        if (_coreEnvelope) {
            _crc.update(bytes);
        }
    }
    std::size_t position{};
    const auto maximum = _transfer.bufferLength().toSizeTOrThrow();
    while (position < bytes.size()) {
        const auto part = bytes.subspan(position, std::min(maximum, bytes.size() - position));
        _write(part);
        position += part.size();
        _progress.transferred.outputLength =
            _progress.transferred.outputLength.addedOrThrow(unit::ByteLength::fromSizeT(part.size()));
        notify();
    }
}

void CompressionTransferOperation::compressCore() {
    auto header = mem::ByteWriter{};
    header.writeUInt32(0x43424c45U)
        .writeUInt8(2U)
        .writeUInt8(static_cast<uint8_t>(_codec.algorithm().toRawValue()))
        .writeUInt16(0U);
    _target(header.toByteBlock().span());
    mem::ByteBlockEditor pending;
    pending.reserve(unit::ByteLength{65536U});
    uint64_t payloadLength{};
    const auto chunk = [&]() {
        auto length = mem::ByteWriter{};
        length.writeUInt32(static_cast<uint32_t>(pending.length().toRawValue()));
        _target(length.toByteBlock().span());
        _target(pending.span());
        pending.clear();
    };
    _codec.compressPayload(
        _source,
        [&](mem::ConstByteSpan bytes) -> void {
            payloadLength =
                unit::ByteLength{payloadLength}.addedOrThrow(unit::ByteLength::fromSizeT(bytes.size())).toRawValue();
            while (!bytes.empty()) {
                const auto count = std::min(bytes.size(), 65536U - pending.length().toSizeT());
                pending.append(bytes.first(count));
                bytes = bytes.subspan(count);
                if (pending.length().toRawValue() == 65536U) {
                    chunk();
                }
            }
        },
        _transfer,
        _encode);
    if (!pending.isEmpty()) {
        chunk();
    }
    _progress.phase = CompressionPhase::Finalizing;
    notify();
    auto trailer = mem::ByteWriter{};
    trailer.writeUInt32(0U)
        .writeUInt64(_progress.transferred.inputLength.toRawValue())
        .writeUInt64(payloadLength)
        .writeUInt32(_crc.value());
    _target(trailer.toByteBlock().span());
}

void CompressionTransferOperation::decompressCore() {
    if (_source.little(4U) != 0x43424c45U) {
        throw CompressionError{CompressionErrorReason::MalformedData, "Invalid Core magic."_el};
    }
    if (_source.little(1U) != 2U) {
        throw CompressionError{CompressionErrorReason::UnsupportedEnvelopeVersion, "Unsupported Core version."_el};
    }
    if (_source.little(1U) != static_cast<uint8_t>(_codec.algorithm().toRawValue())) {
        throw CompressionError{CompressionErrorReason::AlgorithmMismatch, "Core algorithm mismatch."_el};
    }
    if (_source.little(2U) != 0U) {
        throw CompressionError{CompressionErrorReason::UnsupportedFeature, "Unsupported Core flags."_el};
    }
    uint64_t payloadLength{};
    bool ended{};
    auto framed = CodecReader{[&]() -> mem::ByteBlock {
        if (ended) {
            return {};
        }
        const auto length = _source.little(4U);
        if (length == 0U) {
            ended = true;
            return {};
        }
        if (length > 65536U) {
            throw CompressionError{CompressionErrorReason::MalformedData, "Core chunk exceeds 64 KiB."_el};
        }
        auto block = _source.block(static_cast<std::size_t>(length));
        if (block.length().toRawValue() != length) {
            throw CompressionError{CompressionErrorReason::MalformedData, "Truncated Core chunk."_el};
        }
        payloadLength = unit::ByteLength{payloadLength}.addedOrThrow(block.length()).toRawValue();
        return block;
    }};
    mem::ByteBlock buffered;
    if (!_codec.supportsStreaming()) {
        auto bytes = mem::ByteBlockEditor{};
        while (!framed.atEnd()) {
            auto block = framed.block(65536U);
            if (bytes.length().addedOrThrow(block.length()) > _transfer.maximumBufferedLength()) {
                throw err::OutOfRangeError{"Core fallback input limit exceeded."_el};
            }
            bytes.append(block.span());
        }
        buffered = bytes;
    } else {
        _codec.decompressPayload(framed, _target, _transfer, _decode);
    }
    framed.requireEnd();
    const auto originalLength = _source.little(8U);
    const auto encodedLength = _source.little(8U);
    const auto expectedCrc = _source.little(4U);
    if (!_codec.supportsStreaming()) {
        auto options = _decode;
        options.setExpectedOutputLength(unit::ByteLength{originalLength});
        bool readOnce{};
        auto memory = CodecReader{[&]() -> mem::ByteBlock {
            if (readOnce) {
                return {};
            }
            readOnce = true;
            return buffered;
        }};
        _codec.decompressPayload(memory, _target, _transfer, options);
    }
    if (originalLength != _uncompressed || encodedLength != payloadLength || expectedCrc != _crc.value()) {
        throw CompressionError{CompressionErrorReason::LengthMismatch, "Core trailer integrity validation failed."_el};
    }
}

}
