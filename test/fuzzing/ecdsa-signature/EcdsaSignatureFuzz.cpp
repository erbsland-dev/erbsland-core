// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/cryptology/keys/PublicKey.hpp>
#include <erbsland/cryptology/x509/X509AlgorithmIdentifier.hpp>
#include <erbsland/err/Exception.hpp>
#include <erbsland/mem/ByteBlock.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

class EcdsaSignatureFuzzInput final {
public:
    explicit EcdsaSignatureFuzzInput(const std::span<const uint8_t> data) : _data{data} {}

    [[nodiscard]] auto run() const -> int {
        // The four two-octet lengths bound SPKI, AlgorithmIdentifier, message, and ECDSA-Sig-Value independently.
        if (_data.size() < headerSize) {
            return 0;
        }
        const auto lengths = std::array{
            lengthAt(0U),
            lengthAt(2U),
            lengthAt(4U),
            lengthAt(6U),
        };
        if (lengths[0] > maximumPublicKeySize || lengths[1] > maximumAlgorithmSize || lengths[2] > maximumMessageSize ||
            lengths[3] > maximumSignatureSize) {
            return 0;
        }
        auto expectedSize = headerSize;
        for (const auto length : lengths) {
            expectedSize += length;
        }
        if (expectedSize != _data.size()) {
            return 0;
        }

        auto offset = headerSize;
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
        } catch (const erbsland::err::Exception &) {
            // Malformed, unsupported, confused, and resource-bound inputs are documented parse failures.
        }
        return 0;
    }

private:
    [[nodiscard]] auto lengthAt(const std::size_t offset) const -> std::size_t {
        return (static_cast<std::size_t>(_data[offset]) << 8U) | static_cast<std::size_t>(_data[offset + 1U]);
    }

    [[nodiscard]] auto blockAt(const std::size_t offset, const std::size_t size) const -> erbsland::mem::ByteBlock {
        return erbsland::mem::ByteBlock::fromVector(
            std::vector<uint8_t>{
                _data.begin() + static_cast<std::ptrdiff_t>(offset),
                _data.begin() + static_cast<std::ptrdiff_t>(offset + size)});
    }

private:
    static constexpr auto headerSize = std::size_t{8U};
    static constexpr auto maximumPublicKeySize = std::size_t{160U};
    static constexpr auto maximumAlgorithmSize = std::size_t{64U};
    static constexpr auto maximumMessageSize = std::size_t{4096U};
    static constexpr auto maximumSignatureSize = std::size_t{128U};

    std::span<const uint8_t> _data;
};

extern "C" auto LLVMFuzzerTestOneInput(const uint8_t *data, const std::size_t size) -> int {
    return EcdsaSignatureFuzzInput{std::span<const uint8_t>{data, size}}.run();
}
