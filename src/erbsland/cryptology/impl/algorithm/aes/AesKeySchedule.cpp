// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "AesKeySchedule.hpp"

#include "AesOperations.hpp"

#include "../../../../err/ParameterFailure.hpp"
#include "../../../../text/Literals.hpp"

namespace erbsland::cryptology::impl {

using namespace mem;
using namespace unit;
using namespace text::literals;

AesKeySchedule::AesKeySchedule(const ConstByteSpan key) {
    if (key.size() == 16U) {
        _roundCount = 10U;
        _keyWordCount = 4U;
    } else if (key.size() == 32U) {
        _roundCount = 14U;
        _keyWordCount = 8U;
    } else {
        throw err::ParameterFailure{"AES requires a 16-byte or 32-byte key."_el, "key"_el};
    }
    expand(key);
}

AesKeySchedule::~AesKeySchedule() noexcept {
    secureErase();
}

void AesKeySchedule::secureErase() noexcept {
    _roundKeys.secureErase();
    _roundCount = 0U;
    _keyWordCount = 0U;
}

auto AesKeySchedule::roundKey(const std::size_t round) const noexcept -> ByteArray<16> {
    auto result = ByteArray<16>{};
    if (round <= _roundCount) {
        result.overwrite(_roundKeys.span(ByteIndex{round * 16U, ByteIndex::Unchecked{}}, ByteCount{16U}));
    }
    return result;
}

void AesKeySchedule::expand(const ConstByteSpan key) noexcept {
    _roundKeys.overwrite(key);
    const auto totalWordCount = 4U * (_roundCount + 1U);
    auto roundConstant = Byte{0x01U};

    // FIPS 197, Section 5.2: each new word derives from the preceding word and the word Nk positions earlier.
    for (auto word = _keyWordCount; word < totalWordCount; ++word) {
        auto temporary = ByteArray<4>{};
        temporary.length().forEach([&](const ByteIndex byte) -> void {
            temporary.set(
                byte,
                _roundKeys.get(
                    ByteIndex{(word - 1U) * 4U, ByteIndex::Unchecked{}}.uncheckedAdvanced(byte.distanceFromZero())));
        });
        if (word % _keyWordCount == 0U) {
            // FIPS 197, Section 5.2: RotWord, SubWord, and Rcon at every Nk-th word.
            const auto first = temporary.get(ByteIndex{});
            temporary.set(ByteIndex{}, aes::substitute(temporary.get(ByteIndex{1U})) ^ roundConstant);
            temporary.set(ByteIndex{1U}, aes::substitute(temporary.get(ByteIndex{2U})));
            temporary.set(ByteIndex{2U}, aes::substitute(temporary.get(ByteIndex{3U})));
            temporary.set(ByteIndex{3U}, aes::substitute(first));
            roundConstant = aes::multiply(roundConstant, Byte{0x02U});
        } else if (_keyWordCount == 8U && word % _keyWordCount == 4U) {
            // FIPS 197, Section 5.2: AES-256 applies SubWord to word positions congruent to four modulo Nk.
            temporary.length().forEach(
                [&](const ByteIndex byte) -> void { temporary.set(byte, aes::substitute(temporary.get(byte))); });
        }
        temporary.length().forEach([&](const ByteIndex byte) -> void {
            const auto previous = _roundKeys.get(
                ByteIndex{(word - _keyWordCount) * 4U, ByteIndex::Unchecked{}}.uncheckedAdvanced(
                    byte.distanceFromZero()));
            _roundKeys.set(
                ByteIndex{word * 4U, ByteIndex::Unchecked{}}.uncheckedAdvanced(byte.distanceFromZero()),
                previous ^ temporary.get(byte));
        });
        temporary.secureErase();
    }
}

}
