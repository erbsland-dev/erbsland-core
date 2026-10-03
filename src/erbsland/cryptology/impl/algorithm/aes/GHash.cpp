// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "GHash.hpp"

#include "../../../../mem/Endianness.hpp"

#include <algorithm>
#include <utility>

namespace erbsland::cryptology::impl {

using namespace unit;

using namespace mem;

GHash::GHash(const GaloisMultiplier::Block &hashSubkey, std::unique_ptr<GaloisMultiplier> multiplier) noexcept :
    _hashSubkey{hashSubkey}, _multiplier{std::move(multiplier)} {
}

GHash::~GHash() noexcept {
    secureErase();
}

void GHash::addAuthenticatedData(const ConstByteSpan data) noexcept {
    _authenticatedDataLength += static_cast<uint64_t>(data.size());
    add(data);
}

void GHash::addCiphertext(const ConstByteSpan data) noexcept {
    if (!_ciphertextStarted) {
        // SP 800-38D, Section 7.1: AAD and ciphertext are independently zero-padded before concatenation.
        finishPartialBlock();
        _ciphertextStarted = true;
    }
    _ciphertextLength += static_cast<uint64_t>(data.size());
    add(data);
}

auto GHash::finalize() noexcept -> GaloisMultiplier::Block {
    if (_finalized) {
        return _accumulator;
    }
    if (!_ciphertextStarted) {
        finishPartialBlock();
        _ciphertextStarted = true;
    }
    finishPartialBlock();

    // SP 800-38D, Section 7.1: append 64-bit big-endian AAD and ciphertext lengths measured in bits.
    auto lengthBlock = GaloisMultiplier::Block{};
    lengthBlock.setIntegerOrThrow(ByteIndex{}, _authenticatedDataLength * 8U, Endianness::Big);
    lengthBlock.setIntegerOrThrow(ByteIndex{8U}, _ciphertextLength * 8U, Endianness::Big);
    processBlock(lengthBlock);
    lengthBlock.secureErase();
    _finalized = true;
    return _accumulator;
}

void GHash::secureErase() noexcept {
    _hashSubkey.secureErase();
    _accumulator.secureErase();
    _partial.secureErase();
    _partialLength = ByteCount{0U};
    _authenticatedDataLength = 0U;
    _ciphertextLength = 0U;
    _ciphertextStarted = false;
    _finalized = true;
}

void GHash::add(ConstByteSpan data) noexcept {
    auto offset = std::size_t{};
    while (offset < data.size()) {
        const auto available = ByteCount{16U} - _partialLength;
        const auto count = std::min(available, ByteCount::fromSizeT(data.size() - offset));
        _partial.overwrite(ByteIndex::end(_partialLength), ConstByteSpan{data.data() + offset, count.toSizeT()});
        _partialLength += count;
        offset += count.toSizeT();
        if (_partialLength == ByteCount{16U}) {
            processBlock(_partial);
            _partial.fill({});
            _partialLength = ByteCount::zero();
        }
    }
}

void GHash::finishPartialBlock() noexcept {
    if (!_partialLength.isZero()) {
        processBlock(_partial);
        _partial.fill({});
        _partialLength = ByteCount{0U};
    }
}

void GHash::processBlock(const GaloisMultiplier::Block &block) noexcept {
    // SP 800-38D, Equation 2: Xi = (Xi-1 XOR Yi) * H.
    _accumulator ^= block;
    _accumulator = _multiplier->multiply(_accumulator, _hashSubkey);
}

}
