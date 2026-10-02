// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "StreamFixture.hpp"

#include <erbsland/cryptology/impl/algorithm/chacha20/Poly1305Factory.hpp>
#include <erbsland/cryptology/impl/algorithm/chacha20/PortableChaCha20Backend.hpp>
#include <erbsland/cryptology/impl/algorithm/chacha20/PortablePoly1305.hpp>
#include <erbsland/cryptology/impl/symmetric/AesGcmState.hpp>
#include <erbsland/cryptology/impl/symmetric/ChaCha20Poly1305State.hpp>
#include <erbsland/err/LogicError.hpp>
#include <erbsland/mem/Byte.hpp>
#include <erbsland/mem/ByteBuffer.hpp>
#include <erbsland/random/Random.hpp>
#include <erbsland/text/String.hpp>
#include <erbsland/unit/ByteLength.hpp>

namespace app::constant_time {

using namespace el::text::literals;

StreamFixture::StreamFixture(
    el::Random &random, const bool population, std::size_t size, StreamOperation operation, bool automatic) :
    _size{size}, _operation{operation}, _automatic{automatic} {
    auto key = input(random, population, 32);
    auto nonce = el::ByteBuffer{el::ByteLength{12}};
    auto data = el::ByteBuffer{el::ByteLength{size}, el::Byte{0x33U}};
    auto worker = automatic ? ci::createChaCha20Backend(key.span(), nonce.span())
                            : ci::createPortableChaCha20Backend(key.span(), nonce.span());
    require(
        worker->generateBlocks(1, 4) ==
        ci::createPortableChaCha20Backend(key.span(), nonce.span())->generateBlocks(1, 4));
    if (operation == StreamOperation::Poly1305) {
        auto mac = _automatic ? ci::createPoly1305(key.span()) : ci::createPortablePoly1305(key.span());
        auto reference = ci::createPortablePoly1305(key.span());
        mac->update(data.span());
        reference->update(data.span());
        require(mac->finalize() == reference->finalize());
    } else if (operation == StreamOperation::AesGcm) {
        auto encryptor = ci::AesGcmState{key.span(), nonce.span()};
        const auto encrypted = encryptor.transform(data.span(), true);
        const auto tag = encryptor.finalizeTag();
        auto decryptor = ci::AesGcmState{key.span(), nonce.span()};
        require(decryptor.transform(encrypted.span(), false).isEqualConstTime(data.span()));
        require(decryptor.finalizeTag() == tag);
    } else if (operation == StreamOperation::ChaCha20Poly1305) {
        auto encryptor = ci::ChaCha20Poly1305State{key.span(), nonce.span()};
        const auto encrypted = encryptor.transform(data.span(), true);
        const auto tag = encryptor.finalizeTag();
        auto decryptor = ci::ChaCha20Poly1305State{key.span(), nonce.span()};
        require(decryptor.transform(encrypted.span(), false).isEqualConstTime(data.span()));
        require(decryptor.finalizeTag() == tag);
    }
    _key = std::move(key);
    _nonce = std::move(nonce);
    _data = std::move(data);
    _chacha = std::move(worker);
}

template <StreamOperation tOperation>
auto StreamFixture::sample() -> uint64_t {
    constexpr auto operation = tOperation;
    const auto size = _size;

    if constexpr (operation == StreamOperation::ChaCha20) {
        // Include actual tail transformation rather than rounding the requested input
        // down.
        const auto blocks = _chacha->generateBlocks(1, std::min<std::size_t>(4, (size + 63) / 64));
        auto value = checksum(blocks.span().first(std::min<std::size_t>(size, 256)));
        if (size > 256) {
            value += checksum(_chacha->generateBlocks(5, 1).span().first(size - 256));
        }
        return value;
    } else if constexpr (operation == StreamOperation::Poly1305) {
        auto mac = _automatic ? ci::createPoly1305(_key.span()) : ci::createPortablePoly1305(_key.span());
        mac->update(_data.span());
        return checksum(mac->finalize().span());
    } else if constexpr (operation == StreamOperation::AesGcm) {
        auto state = ci::AesGcmState{_key.span(), _nonce.span()};
        const auto output = state.transform(_data.span(), true);
        return checksum(state.finalizeTag().span()) + checksum(output.span());
    } else {
        auto state = ci::ChaCha20Poly1305State{_key.span(), _nonce.span()};
        const auto output = state.transform(_data.span(), true);
        return checksum(state.finalizeTag().span()) + checksum(output.span());
    }
}

auto StreamFixture::measure(const uint64_t repetitions) -> uint64_t {
    switch (_operation) {
    case StreamOperation::ChaCha20:
        return repeat(repetitions, [this]() -> uint64_t { return sample<StreamOperation::ChaCha20>(); });
    case StreamOperation::Poly1305:
        return repeat(repetitions, [this]() -> uint64_t { return sample<StreamOperation::Poly1305>(); });
    case StreamOperation::AesGcm:
        return repeat(repetitions, [this]() -> uint64_t { return sample<StreamOperation::AesGcm>(); });
    case StreamOperation::ChaCha20Poly1305:
        return repeat(repetitions, [this]() -> uint64_t { return sample<StreamOperation::ChaCha20Poly1305>(); });
    }
    throw el::LogicError{"Unknown sample operation."_el};
}

}
