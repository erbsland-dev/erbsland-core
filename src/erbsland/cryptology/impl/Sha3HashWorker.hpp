// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "HashWorker.hpp"
#include "Sha3.hpp"

#include "../../mem/ByteBlock.hpp"

#include <cstdint>
#include <memory>
#include <vector>

namespace erbsland::cryptology::impl {

/// Adapt one SHA3 implementation to the hash worker contract.
/// @tparam tSha3 The concrete SHA3 implementation.
/// @tparam tAlgorithm The public algorithm value.
/// @tested{Sha3ValidationTest}
template <typename tSha3, HashAlgorithm::Value tAlgorithm>
class Sha3HashWorker final : public HashWorker {
public:
    Sha3HashWorker() = default;

public: // implement HashWorker
    [[nodiscard]] auto clone() const -> std::shared_ptr<HashWorker> override {
        return std::make_shared<Sha3HashWorker>(*this);
    }
    [[nodiscard]] auto algorithm() const noexcept -> HashAlgorithm override { return tAlgorithm; }
    void reset() override { _sha3.reset(); }
    void update(const std::span<const std::byte> data) override { _sha3.update(data); }
    [[nodiscard]] auto finalize() -> mem::ByteBlock override {
        const auto digest = _sha3.digest();
        auto bytes = std::vector<uint8_t>{};
        bytes.reserve(digest.size());
        for (const auto byte : digest) {
            bytes.push_back(std::to_integer<uint8_t>(byte));
        }
        return mem::ByteBlock{bytes};
    }

private:
    tSha3 _sha3; ///< The adapted SHA3 state.
};

using Sha3_256HashWorker = Sha3HashWorker<Sha3_256, HashAlgorithm::Sha3_256>;
using Sha3_384HashWorker = Sha3HashWorker<Sha3_384, HashAlgorithm::Sha3_384>;
using Sha3_512HashWorker = Sha3HashWorker<Sha3_512, HashAlgorithm::Sha3_512>;

}
