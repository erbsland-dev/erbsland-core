// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "EcdsaSignatureFuzz.hpp"

#include <erbsland/ByteBlock.hpp>
#include <erbsland/PublicKey.hpp>
#include <erbsland/RuntimeError.hpp>
#include <erbsland/X509AlgorithmIdentifier.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

auto EcdsaSignatureFuzz::run() const -> int {
    // The four two-octet lengths bound SPKI, AlgorithmIdentifier, message, and ECDSA-Sig-Value independently.
    if (_data.size() < cHeaderSize) {
        return 0;
    }
    const auto lengths = std::array{
        lengthAt(0U),
        lengthAt(2U),
        lengthAt(4U),
        lengthAt(6U),
    };
    if (lengths[0] > cMaximumPublicKeySize || lengths[1] > cMaximumAlgorithmSize || lengths[2] > cMaximumMessageSize ||
        lengths[3] > cMaximumSignatureSize) {
        return 0;
    }
    auto expectedSize = cHeaderSize;
    for (const auto length : lengths) {
        expectedSize += length;
    }
    if (expectedSize != _data.size()) {
        return 0;
    }

    auto offset = cHeaderSize;
    const auto publicKeyDer = blockAt(offset, lengths[0]);
    offset += lengths[0];
    const auto algorithmDer = blockAt(offset, lengths[1]);
    offset += lengths[1];
    const auto message = blockAt(offset, lengths[2]);
    offset += lengths[2];
    const auto signature = blockAt(offset, lengths[3]);

    const auto publicKey = erbsland::cryptology::PublicKey::fromDer(publicKeyDer);
    const auto algorithm = erbsland::cryptology::X509AlgorithmIdentifier::fromDer(algorithmDer);
    if (publicKey.isEmpty() || algorithm.isEmpty()) {
        return 0;
    }
    try {
        static_cast<void>(publicKey.verifySignature(algorithm, message.span(), signature.span()));
    } catch (const erbsland::err::RuntimeError &) {
        // Malformed, unsupported, confused, and resource-bound inputs are documented parse failures.
    }
    return 0;
}

auto EcdsaSignatureFuzz::lengthAt(const std::size_t offset) const -> std::size_t {
    return (static_cast<std::size_t>(_data[offset]) << 8U) | static_cast<std::size_t>(_data[offset + 1U]);
}

auto EcdsaSignatureFuzz::blockAt(const std::size_t offset, const std::size_t size) const -> erbsland::mem::ByteBlock {
    return erbsland::mem::ByteBlock::fromVector(
        std::vector<uint8_t>{
            _data.begin() + static_cast<std::ptrdiff_t>(offset),
            _data.begin() + static_cast<std::ptrdiff_t>(offset + size)});
}

extern "C" auto LLVMFuzzerTestOneInput(const uint8_t *data, const std::size_t size) -> int {
    return EcdsaSignatureFuzz{std::span<const uint8_t>{data, size}}.run();
}
