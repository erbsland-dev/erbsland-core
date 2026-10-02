// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "AuthenticationFixture.hpp"

#include <erbsland/cryptology/HashAlgorithm.hpp>
#include <erbsland/cryptology/Hkdf.hpp>
#include <erbsland/cryptology/Hmac.hpp>
#include <erbsland/err/LogicError.hpp>
#include <erbsland/mem/Byte.hpp>
#include <erbsland/mem/ByteBuffer.hpp>
#include <erbsland/random/Random.hpp>
#include <erbsland/text/String.hpp>
#include <erbsland/unit/ByteIndex.hpp>
#include <erbsland/unit/ByteLength.hpp>

namespace app::constant_time {

using namespace el::text::literals;

AuthenticationFixture::AuthenticationFixture(
    el::Random &random, const bool population, el::HashAlgorithm algorithm, AuthenticationOperation operation) :
    _algorithm{algorithm}, _operation{operation} {
    auto key = input(random, operation != AuthenticationOperation::HmacVerify && population, 48);
    auto data = el::ByteBuffer{el::ByteLength{64}, el::Byte{0x33U}};
    auto hmac = std::make_unique<el::Hmac>(algorithm, key.span());
    hmac->update(data.span());
    const auto tag = hmac->finalize();
    require(hmac->verify(tag.span()));
    if (operation == AuthenticationOperation::HkdfExtract) {
        auto reference = el::Hmac{algorithm, data.span()};
        reference.update(key.span());
        require(el::Hkdf{algorithm}.extract(key.span(), data.span()) == reference.finalize());
    }
    if (operation == AuthenticationOperation::HkdfExpand) {
        require(el::Hkdf{algorithm}.expand(key.span(), data.span(), el::ByteLength{64}).length() == el::ByteLength{64});
    }
    if (operation == AuthenticationOperation::HmacVerify) {
        key = el::ByteBuffer{tag.span()};
        const auto index = population ? key.length().toSizeT() - 1 : 0;
        key.set(el::ByteIndex{index}, key.get(el::ByteIndex{index}) ^ el::Byte{1U});
        require(!hmac->verify(key.span()));
    }
    _key = std::move(key);
    _data = std::move(data);
    _hmac = std::move(hmac);
}

template <AuthenticationOperation tOperation>
auto AuthenticationFixture::sample() -> uint64_t {
    constexpr auto operation = tOperation;

    if constexpr (operation == AuthenticationOperation::HmacCalculate) {
        _hmac->reset();
        _hmac->update(_data.span());
        return checksum(_hmac->finalize().span());
    } else if constexpr (operation == AuthenticationOperation::HmacVerify) {
        return _hmac->verify(_key.span());
    } else {
        const auto hkdf = el::Hkdf{_algorithm};
        if constexpr (operation == AuthenticationOperation::HkdfExtract) {
            return checksum(hkdf.extract(_key.span(), _data.span()).span());
        } else {
            return checksum(hkdf.expand(_key.span(), _data.span(), el::ByteLength{64}).span());
        }
    }
}

auto AuthenticationFixture::measure(const uint64_t repetitions) -> uint64_t {
    switch (_operation) {
    case AuthenticationOperation::HmacCalculate:
        return repeat(repetitions, [this]() -> uint64_t { return sample<AuthenticationOperation::HmacCalculate>(); });
    case AuthenticationOperation::HmacVerify:
        return repeat(repetitions, [this]() -> uint64_t { return sample<AuthenticationOperation::HmacVerify>(); });
    case AuthenticationOperation::HkdfExtract:
        return repeat(repetitions, [this]() -> uint64_t { return sample<AuthenticationOperation::HkdfExtract>(); });
    case AuthenticationOperation::HkdfExpand:
        return repeat(repetitions, [this]() -> uint64_t { return sample<AuthenticationOperation::HkdfExpand>(); });
    }
    throw el::LogicError{"Unknown sample operation."_el};
}

}
