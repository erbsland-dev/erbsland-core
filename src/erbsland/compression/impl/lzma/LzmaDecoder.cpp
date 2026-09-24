// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "LzmaDecoder.hpp"

#include "../../../err/OutOfRangeError.hpp"
#include "../../../text/Literals.hpp"
#include "../../CompressionError.hpp"

#include <algorithm>
#include <span>

namespace erbsland::compression::impl {

using namespace text::literals;

LzmaDecoder::LzmaDecoder(
    CodecReader &input,
    const uint32_t dictionarySize,
    const uint8_t lc,
    const uint8_t lp,
    const uint8_t pb,
    const std::optional<std::size_t> expected,
    const std::size_t maximum,
    CodecOutput::Write output) :
    _decoder{input},
    _dictionarySize{std::max(dictionarySize, cMinimumDictionarySize)},
    _lc{lc},
    _positionMask{positionMask(pb)},
    _literalPositionMask{positionMask(lp)},
    _expected{expected},
    _maximum{maximum},
    _output{std::move(output), std::max(dictionarySize, cMinimumDictionarySize), unit::ByteLength::fromSizeT(maximum)} {
    if (lc + lp > 4U) {
        throw CompressionError{
            CompressionErrorReason::UnsupportedFeature,
            "LZMA literal context settings exceed the supported workspace."_el};
    }
}

auto LzmaDecoder::positionMask(const uint8_t bits) -> uint32_t {
    if (bits > 4U) {
        throw CompressionError{CompressionErrorReason::MalformedData, "Invalid LZMA position properties."_el};
    }
    return (1U << bits) - 1U;
}

void LzmaDecoder::decode() {
    while (!_expected.has_value() || _output.length().toSizeT() < *_expected || !_decoder.isFinished()) {
        const auto posState = _output.length().toSizeT() & _positionMask;
        if (_decoder.decodeBit(_probabilities.isMatch[_state * cPositionStateCount + posState]) == 0U) {
            decodeLiteral();
            updateLiteralState();
            continue;
        }
        if (_decoder.decodeBit(_probabilities.isRep[_state]) != 0U) {
            if (decodeRep(posState)) {
                continue;
            }
        } else {
            _rep3 = _rep2;
            _rep2 = _rep1;
            _rep1 = _rep0;
            const auto length =
                decodeLength(_probabilities.matchLzmaLengthProbabilities, posState) + cMinimumMatchLength;
            _state = _state < 7U ? 7U : 10U;
            _rep0 = decodeDistance(length);
            if (_rep0 == cEndMarkerDistance) {
                _sawEndMarker = true;
                break;
            }
            copyMatch(length);
            continue;
        }
        const auto length = decodeLength(_probabilities.repLzmaLengthProbabilities, posState) + cMinimumMatchLength;
        _state = _state < 7U ? 8U : 11U;
        copyMatch(length);
    }
    if (_expected.has_value() && _output.length().toSizeT() != *_expected) {
        throw CompressionError{CompressionErrorReason::LengthMismatch, "LZMA output length does not match."_el};
    }
    if (!_expected.has_value() && !_sawEndMarker) {
        throw CompressionError{CompressionErrorReason::MalformedData, "LZMA payload has no end marker."_el};
    }
    if (!_decoder.isFinished()) {
        throw CompressionError{CompressionErrorReason::MalformedData, "LZMA range coder did not finish cleanly."_el};
    }
    _output.flush();
}

void LzmaDecoder::append(const uint8_t value) {
    if (_expected.has_value() && _output.length().toSizeT() >= *_expected) {
        throw CompressionError{CompressionErrorReason::LengthMismatch, "LZMA output exceeds its expected length."_el};
    }
    if (_output.length().toSizeT() >= _maximum) {
        throw err::OutOfRangeError{"LZMA output exceeds the configured maximum."_el};
    }
    _output.append(mem::Byte{value});
}

void LzmaDecoder::decodeLiteral() {
    const auto outputSize = _output.length().toSizeT();
    const auto previous =
        _output.isEmpty() ? uint8_t{} : _output.getOrThrow(unit::ByteIndex::fromSizeT(outputSize - 1U)).toUInt8();
    const auto context = ((outputSize & _literalPositionMask) << _lc) + (previous >> (8U - _lc));
    const auto probabilities = std::span{_probabilities.literals}.subspan(context * cLiteralTreeSize, cLiteralTreeSize);
    auto symbol = uint32_t{1U};
    if (_state >= 7U) {
        if (_rep0 >= outputSize) {
            throw CompressionError{CompressionErrorReason::MalformedData, "Invalid LZMA literal reference."_el};
        }
        auto match = _output.getOrThrow(unit::ByteIndex::fromSizeT(outputSize - _rep0 - 1U)).toUInt8();
        do {
            const auto matchBit = (match >> 7U) & 1U;
            match <<= 1U;
            const auto bit = _decoder.decodeBit(probabilities[cLiteralSymbolBase + (matchBit << 8U) + symbol]);
            symbol = (symbol << 1U) | bit;
            if (bit != matchBit) {
                while (symbol < cLiteralSymbolBase) {
                    symbol = (symbol << 1U) | _decoder.decodeBit(probabilities[symbol]);
                }
                break;
            }
        } while (symbol < cLiteralSymbolBase);
    } else {
        while (symbol < cLiteralSymbolBase) {
            symbol = (symbol << 1U) | _decoder.decodeBit(probabilities[symbol]);
        }
    }
    append(static_cast<uint8_t>(symbol));
}

void LzmaDecoder::updateLiteralState() noexcept {
    _state = _state < 4U ? 0U : (_state < 10U ? _state - 3U : _state - 6U);
}

[[nodiscard]] auto LzmaDecoder::decodeRep(const std::size_t posState) -> bool {
    if (_decoder.decodeBit(_probabilities.isRepG0[_state]) == 0U) {
        if (_decoder.decodeBit(_probabilities.isRep0Long[_state * cPositionStateCount + posState]) == 0U) {
            _state = _state < 7U ? 9U : 11U;
            copyMatch(1U);
            return true;
        }
    } else {
        uint32_t distance;
        if (_decoder.decodeBit(_probabilities.isRepG1[_state]) == 0U) {
            distance = _rep1;
        } else {
            if (_decoder.decodeBit(_probabilities.isRepG2[_state]) == 0U) {
                distance = _rep2;
            } else {
                distance = _rep3;
                _rep3 = _rep2;
            }
            _rep2 = _rep1;
        }
        _rep1 = _rep0;
        _rep0 = distance;
    }
    return false;
}

[[nodiscard]] auto LzmaDecoder::decodeLength(LzmaLengthProbabilities &length, const std::size_t posState) -> uint32_t {
    if (_decoder.decodeBit(length.choice) == 0U) {
        return _decoder.decodeTree(std::span{length.low}.subspan(posState * 8U, 8U), 3U);
    }
    if (_decoder.decodeBit(length.choice2) == 0U) {
        return 8U + _decoder.decodeTree(std::span{length.mid}.subspan(posState * 8U, 8U), 3U);
    }
    return 16U + _decoder.decodeTree(length.high, 8U);
}

[[nodiscard]] auto LzmaDecoder::decodeDistance(const uint32_t length) -> uint32_t {
    const auto lengthState = std::min(length - cMinimumMatchLength, 3U);
    const auto slot = _decoder.decodeTree(std::span{_probabilities.posSlot}.subspan(lengthState * 64U, 64U), 6U);
    if (slot < 4U) {
        return slot;
    }
    const auto directBits = static_cast<unsigned>((slot >> 1U) - 1U);
    auto distance = (2U | (slot & 1U)) << directBits;
    if (slot < 14U) {
        distance +=
            decodeCompactReverseTree(std::span{_probabilities.posDecoders}.subspan(distance - slot), directBits);
    } else {
        distance += _decoder.decodeDirect(directBits - 4U) << 4U;
        distance += _decoder.decodeReverseTree(_probabilities.posAlign, 4U);
    }
    return distance;
}

auto LzmaDecoder::decodeCompactReverseTree(const std::span<uint16_t> probabilities, const unsigned bitCount)
    -> uint32_t {
    auto symbol = uint32_t{1U};
    auto result = uint32_t{};
    for (auto index = 0U; index < bitCount; ++index) {
        const auto bit = _decoder.decodeBit(probabilities[symbol - 1U]);
        symbol = (symbol << 1U) | bit;
        result |= bit << index;
    }
    return result;
}

void LzmaDecoder::copyMatch(const uint32_t length) {
    const auto outputLength = _output.length().toSizeT();
    if (_rep0 >= _dictionarySize || _rep0 >= outputLength) {
        throw CompressionError{CompressionErrorReason::MalformedData, "Invalid LZMA match distance."_el};
    }
    if (_expected.has_value() && (outputLength > *_expected || length > *_expected - outputLength)) {
        throw CompressionError{CompressionErrorReason::LengthMismatch, "LZMA output exceeds its expected length."_el};
    }
    if (outputLength > _maximum || length > _maximum - outputLength) {
        throw err::OutOfRangeError{"LZMA output exceeds the configured maximum."_el};
    }
    const auto distance = static_cast<std::size_t>(_rep0) + 1U;
    _output.appendRepeated(
        unit::ByteRange{unit::ByteIndex::fromSizeT(outputLength - distance), unit::ByteLength::fromSizeT(distance)},
        unit::ByteLength::fromSizeT(length));
}

}
