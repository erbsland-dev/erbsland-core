// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PasswordVerifierFixture.hpp"

#include <erbsland/Byte.hpp>
#include <erbsland/ByteBlock.hpp>
#include <erbsland/ByteBuffer.hpp>
#include <erbsland/ByteCount.hpp>
#include <erbsland/ByteIndex.hpp>
#include <erbsland/PasswordHashKey.hpp>
#include <erbsland/PasswordHashPolicy.hpp>
#include <erbsland/Random.hpp>

namespace app::constant_time {

using namespace el::text::literals;

PasswordVerifierFixture::PasswordVerifierFixture(
    [[maybe_unused]] el::Random &random, const bool population, bool keyed) {
    auto raw = el::ByteBuffer{el::ByteCount{32}, el::Byte{0x55U}};
    const auto salt = el::ByteBuffer{el::ByteCount{16}};
    auto key = keyed ? std::make_unique<el::PasswordHashKey>(el::ByteBlock::fromSpan(raw.span())) : nullptr;
    auto record =
        ci::PasswordHashData::create(el::PasswordHashPolicy::recommended(), key.get(), salt.span(), raw.span());
    require(record->matchesVerifier(raw.span(), key.get()));
    auto changedIndex = el::ByteIndex::zero();
    if (population) {
        changedIndex = el::ByteIndex{31U};
    }
    raw.set(changedIndex, el::Byte{0x54U});
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
