// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "LzmaEncoder.hpp"

#include "../../../mem/ByteBlock.hpp"

#include <algorithm>
#include <bit>
#include <span>

namespace erbsland::compression::impl {

LzmaEncoder::LzmaEncoder(const uint32_t dictionarySize, const std::size_t depth) :
    _dictionarySize{dictionarySize}, _depth{depth}, _head(cHashTableSize, cNone) {
}

void LzmaEncoder::encodePart() {
    // encodeStream sizes the chain for this input; links always refer to earlier positions.
    while (_position < _input.size()) {
        auto matchLength = std::size_t{};
        auto matchDistance = std::size_t{};
        if (_position + cMinimumMatchLength <= _input.size()) {
            const auto hashValue = hash(_position);
            auto candidate = _head[hashValue];
            _previousChain[_position] = candidate;
            _head[hashValue] = static_cast<uint32_t>(_position);
            auto attempts = std::size_t{};
            while (candidate != cNone && attempts++ < _depth) {
                const auto distance = _position - candidate;
                if (distance > _dictionarySize) {
                    break;
                }
                auto length = std::size_t{};
                const auto maximum = std::min(cMaximumMatchLength, _input.size() - _position);
                while (length < maximum && _input[candidate + length] == _input[_position + length]) {
                    ++length;
                }
                if (length > matchLength) {
                    matchLength = length;
                    matchDistance = distance;
                    if (length == maximum) {
                        break; // No later candidate can improve this match.
                    }
                }
                candidate = _previousChain[candidate];
            }
        }
        if (matchLength >= cMinimumMatchLength) {
            encodeMatch(static_cast<uint32_t>(matchLength), static_cast<uint32_t>(matchDistance - 1U));
            const auto end = _position + matchLength;
            ++_position;
            while (_position < end) {
                if (_position + cMinimumMatchLength <= _input.size()) {
                    const auto hashValue = hash(_position);
                    _previousChain[_position] = _head[hashValue];
                    _head[hashValue] = static_cast<uint32_t>(_position);
                }
                ++_position;
            }
        } else {
            encodeLiteral(_input[_position].toUInt8());
            ++_position;
        }
    }
}

[[nodiscard]] auto LzmaEncoder::hash(const std::size_t position) const noexcept -> std::size_t {
    const auto value = (_input[position].toUInt32() << 16U) | (_input[position + 1U].toUInt32() << 8U) |
        _input[position + 2U].toUInt32();
    return (value * cHashMultiplier) >> 16U;
}

void LzmaEncoder::encodeLiteral(const uint8_t value) {
    const auto posState = _position & cPositionStateMask;
    _encoder.encodeBit(_probabilities.isMatch[_state * 16U + posState], 0U);
    const auto context = static_cast<std::size_t>(_previous >> 5U);
    const auto probabilities = std::span{_probabilities.literals}.subspan(context * cLiteralTreeSize, cLiteralTreeSize);
    auto symbol = uint32_t{1U};
    if (_state >= 7U && _rep0 < _position) {
        auto match = _input[_position - _rep0 - 1U].toUInt8();
        for (auto bitIndex = 8U; bitIndex != 0U; --bitIndex) {
            const auto matchBit = (match >> 7U) & 1U;
            match <<= 1U;
            const auto bit = (value >> (bitIndex - 1U)) & 1U;
            _encoder.encodeBit(probabilities[cLiteralSymbolBase + (matchBit << 8U) + symbol], bit);
            symbol = (symbol << 1U) | bit;
            if (bit != matchBit) {
                while (--bitIndex != 0U) {
                    const auto remainingBit = (value >> (bitIndex - 1U)) & 1U;
                    _encoder.encodeBit(probabilities[symbol], remainingBit);
                    symbol = (symbol << 1U) | remainingBit;
                }
                break;
            }
        }
    } else {
        for (auto bitIndex = 8U; bitIndex != 0U; --bitIndex) {
            const auto bit = (value >> (bitIndex - 1U)) & 1U;
            _encoder.encodeBit(probabilities[symbol], bit);
            symbol = (symbol << 1U) | bit;
        }
    }
    _previous = value;
    _state = _state < 4U ? 0U : (_state < 10U ? _state - 3U : _state - 6U);
}

void LzmaEncoder::encodeLength(LzmaLengthProbabilities &length, const std::size_t posState, const uint32_t symbol) {
    if (symbol < 8U) {
        _encoder.encodeBit(length.choice, 0U);
        _encoder.encodeTree(std::span{length.low}.subspan(posState * 8U, 8U), 3U, symbol);
    } else {
        _encoder.encodeBit(length.choice, 1U);
        if (symbol < 16U) {
            _encoder.encodeBit(length.choice2, 0U);
            _encoder.encodeTree(std::span{length.mid}.subspan(posState * 8U, 8U), 3U, symbol - 8U);
        } else {
            _encoder.encodeBit(length.choice2, 1U);
            _encoder.encodeTree(length.high, 8U, symbol - 16U);
        }
    }
}

void LzmaEncoder::encodeDistance(const uint32_t distance, const uint32_t length) {
    const auto lengthState = std::min(length - 2U, 3U);
    uint32_t slot;
    if (distance < 4U) {
        slot = distance;
    } else {
        const auto highest = static_cast<uint32_t>(std::bit_width(distance)) - 1U;
        slot = highest * 2U + ((distance >> (highest - 1U)) & 1U);
    }
    _encoder.encodeTree(std::span{_probabilities.posSlot}.subspan(lengthState * 64U, 64U), 6U, slot);
    if (slot < 4U) {
        return;
    }
    const auto directBits = static_cast<unsigned>((slot >> 1U) - 1U);
    const auto base = (2U | (slot & 1U)) << directBits;
    const auto remainder = distance - base;
    if (slot < 14U) {
        encodeCompactReverseTree(std::span{_probabilities.posDecoders}.subspan(base - slot), directBits, remainder);
    } else {
        _encoder.encodeDirect(remainder >> 4U, directBits - 4U);
        _encoder.encodeReverseTree(_probabilities.posAlign, 4U, remainder & 15U);
    }
}

void LzmaEncoder::encodeCompactReverseTree(
    const std::span<uint16_t> probabilities, const unsigned bitCount, uint32_t value) {
    auto symbol = uint32_t{1U};
    for (auto index = 0U; index < bitCount; ++index) {
        const auto bit = value & 1U;
        value >>= 1U;
        _encoder.encodeBit(probabilities[symbol - 1U], bit);
        symbol = (symbol << 1U) | bit;
    }
}

void LzmaEncoder::encodeMatch(const uint32_t length, const uint32_t distance) {
    const auto posState = _position & cPositionStateMask;
    _encoder.encodeBit(_probabilities.isMatch[_state * 16U + posState], 1U);
    _encoder.encodeBit(_probabilities.isRep[_state], 0U);
    encodeLength(_probabilities.matchLzmaLengthProbabilities, posState, length - 2U);
    encodeDistance(distance, length);
    _rep0 = distance;
    _state = _state < 7U ? 7U : 10U;
    _previous = _input[_position + length - 1U].toUInt8();
}

void LzmaEncoder::encodeEndMarker() {
    const auto posState = _position & cPositionStateMask;
    _encoder.encodeBit(_probabilities.isMatch[_state * 16U + posState], 1U);
    _encoder.encodeBit(_probabilities.isRep[_state], 0U);
    encodeLength(_probabilities.matchLzmaLengthProbabilities, posState, 0U);
    _encoder.encodeTree(std::span{_probabilities.posSlot}.first<64U>(), 6U, cEndMarkerSlot);
    _encoder.encodeDirect(cEndMarkerDirectBits, 26U);
    _encoder.encodeReverseTree(_probabilities.posAlign, 4U, 15U);
}

void LzmaEncoder::encodeStream(CodecReader &input, const CodecOutput::Write &output) {
    _encoder.setOutput(output);
    auto history = mem::ByteBlockEditor{};
    history.reserve(unit::ByteLength{65536U});
    _previousChain.reserve(static_cast<std::size_t>(_dictionarySize) + 65536U);
    // Bounded indexes cover a dictionary and one input block.

    while (!input.atEnd()) {
        const auto block = input.block(65536U);
        history.append(block.span());
        _input = history.span();
        _previousChain.resize(_input.size(), cNone);
        encodePart();
        if (input.atEnd()) {
            encodeEndMarker();
            _encoder.finalize();
            return;
        }
        const auto retain = std::min(_input.size(), static_cast<std::size_t>(_dictionarySize));
        const auto removed = _input.size() - retain;
        if (removed) {
            for (auto &value : _head) {
                value = value == cNone || value < removed ? cNone : static_cast<uint32_t>(value - removed);
            }
            for (std::size_t i{}; i < retain; ++i) {
                const auto value = _previousChain[i + removed];
                _previousChain[i] = value == cNone || value < removed ? cNone : static_cast<uint32_t>(value - removed);
            }
        }
        _position -= removed;
        history.remove(unit::ByteRange{unit::ByteIndex{}, unit::ByteLength::fromSizeT(removed)});
    }
    encodeEndMarker();
    _encoder.finalize();
}

}
