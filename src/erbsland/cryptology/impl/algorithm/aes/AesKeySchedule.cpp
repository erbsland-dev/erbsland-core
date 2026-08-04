// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "AesKeySchedule.hpp"

#include "AesOperations.hpp"

#include "../../../../err/ParameterError.hpp"
#include "../../../../text/Literals.hpp"

namespace erbsland::cryptology::impl {

using namespace text::literals;

AesKeySchedule::AesKeySchedule(const mem::ConstByteSpan key) {
    if (key.size() == 16U) {
        _roundCount = 10U;
        _keyWordCount = 4U;
    } else if (key.size() == 32U) {
        _roundCount = 14U;
        _keyWordCount = 8U;
    } else {
        throw err::ParameterError{"AES requires a 16-byte or 32-byte key."_el, "key"_el};
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

auto AesKeySchedule::roundKey(const std::size_t round) const noexcept -> mem::ByteArray<16> {
    auto result = mem::ByteArray<16>{};
    if (round <= _roundCount) {
        result.overwrite(_roundKeys.span(unit::ByteIndex{round * 16U}, unit::ByteLength{16U}));
    }
    return result;
}

void AesKeySchedule::expand(const mem::ConstByteSpan key) noexcept {
    _roundKeys.overwrite(key);
    const auto totalWordCount = 4U * (_roundCount + 1U);
    auto roundConstant = mem::Byte{0x01U};

    // FIPS 197, Section 5.2: each new word derives from the preceding word and the word Nk positions earlier.
    for (auto word = _keyWordCount; word < totalWordCount; ++word) {
        auto temporary = mem::ByteArray<4>{};
        for (auto byte = 0U; byte < 4U; ++byte) {
            temporary.set(unit::ByteIndex{byte}, _roundKeys.get(unit::ByteIndex{(word - 1U) * 4U + byte}));
        }
        if (word % _keyWordCount == 0U) {
            // FIPS 197, Section 5.2: RotWord, SubWord, and Rcon at every Nk-th word.
            const auto first = temporary.get(unit::ByteIndex{});
            temporary.set(unit::ByteIndex{}, aes::substitute(temporary.get(unit::ByteIndex{1U})) ^ roundConstant);
            temporary.set(unit::ByteIndex{1U}, aes::substitute(temporary.get(unit::ByteIndex{2U})));
            temporary.set(unit::ByteIndex{2U}, aes::substitute(temporary.get(unit::ByteIndex{3U})));
            temporary.set(unit::ByteIndex{3U}, aes::substitute(first));
            roundConstant = aes::multiply(roundConstant, mem::Byte{0x02U});
        } else if (_keyWordCount == 8U && word % _keyWordCount == 4U) {
            // FIPS 197, Section 5.2: AES-256 applies SubWord to word positions congruent to four modulo Nk.
            for (auto byte = unit::ByteIndex{}; byte < temporary.endIndex(); ++byte) {
                temporary.set(byte, aes::substitute(temporary.get(byte)));
            }
        }
        for (auto byte = 0U; byte < 4U; ++byte) {
            const auto previous = _roundKeys.get(unit::ByteIndex{(word - _keyWordCount) * 4U + byte});
            _roundKeys.set(unit::ByteIndex{word * 4U + byte}, previous ^ temporary.get(unit::ByteIndex{byte}));
        }
        temporary.secureErase();
    }
}

}
