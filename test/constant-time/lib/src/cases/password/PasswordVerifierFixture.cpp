// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "PasswordVerifierFixture.hpp"

#include <erbsland/cryptology/PasswordHashKey.hpp>
#include <erbsland/cryptology/PasswordHashPolicy.hpp>
#include <erbsland/mem/Byte.hpp>
#include <erbsland/mem/ByteBlock.hpp>
#include <erbsland/mem/ByteBuffer.hpp>
#include <erbsland/random/Random.hpp>
#include <erbsland/unit/ByteIndex.hpp>
#include <erbsland/unit/ByteLength.hpp>

namespace app::constant_time {

using namespace el::text::literals;

PasswordVerifierFixture::PasswordVerifierFixture(
    [[maybe_unused]] el::Random &random, const bool population, bool keyed) {
    auto raw = el::ByteBuffer{el::ByteLength{32}, el::Byte{0x55U}};
    const auto salt = el::ByteBuffer{el::ByteLength{16}};
    auto key = keyed ? std::make_unique<el::PasswordHashKey>(el::ByteBlock::fromSpan(raw.span())) : nullptr;
    auto record =
        ci::PasswordHashData::create(el::PasswordHashPolicy::recommended(), key.get(), salt.span(), raw.span());
    require(record->matchesVerifier(raw.span(), key.get()));
    raw.set(el::ByteIndex{population ? 31U : 0U}, el::Byte{0x54U});
    require(!record->matchesVerifier(raw.span(), key.get()));
    _raw = std::move(raw);
    _key = std::move(key);
    _record = std::move(record);
}

auto PasswordVerifierFixture::sample() -> uint64_t {
    return _record->matchesVerifier(_raw.span(), _key.get());
}

auto PasswordVerifierFixture::measure(const uint64_t repetitions) -> uint64_t {
    return repeat(repetitions, [this]() -> uint64_t { return sample(); });
}

}
