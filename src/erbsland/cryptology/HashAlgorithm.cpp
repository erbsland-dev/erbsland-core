// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "HashAlgorithm.hpp"

#include "../err/ParseError.hpp"
#include "../text/Literals.hpp"
#include "../util/List.hpp"

#include <array>

namespace erbsland::cryptology {

using namespace text::literals;

auto HashAlgorithm::digestSize() const noexcept -> unit::ByteLength {
    switch (_value) {
    case Sha3_256:
    case Sha2_256:
        return unit::ByteLength{32U};
    case Sha3_384:
    case Sha2_384:
        return unit::ByteLength{48U};
    case Sha3_512:
    case Sha2_512:
        return unit::ByteLength{64U};
    case Sha1:
        return unit::ByteLength{20U};
    case Md5:
        return unit::ByteLength{16U};
    }
    return unit::ByteLength::zero();
}

auto HashAlgorithm::status() const noexcept -> CryptographicStatus {
    switch (_value) {
    case Sha3_256:
    case Sha3_384:
    case Sha3_512:
    case Sha2_256:
    case Sha2_384:
    case Sha2_512:
        return CryptographicStatus::Acceptable;
    case Sha1:
    case Md5:
        return CryptographicStatus::Disallowed;
    }
    return CryptographicStatus::Disallowed;
}

auto HashAlgorithm::security() const noexcept -> CryptographicSecurity {
    switch (_value) {
    case Sha3_256:
    case Sha2_256:
    case Sha1:
    case Md5:
        return CryptographicSecurity::Standard;
    case Sha3_384:
    case Sha3_512:
    case Sha2_384:
    case Sha2_512:
        return CryptographicSecurity::High;
    }
    return CryptographicSecurity::Standard;
}

auto HashAlgorithm::throughput() const noexcept -> HashThroughput {
    switch (_value) {
    case Sha3_256:
    case Sha2_256:
        return HashThroughput::High;
    case Sha3_384:
    case Sha2_384:
        return HashThroughput::Medium;
    case Sha3_512:
    case Sha2_512:
    case Sha1:
    case Md5:
        return HashThroughput::Low;
    }
    return HashThroughput::Low;
}

auto HashAlgorithm::isSafe() const noexcept -> bool {
    return status() == CryptographicStatus::Acceptable && security() >= CryptographicSecurity::Standard;
}

auto HashAlgorithm::matches(const HashRequirements &requirements) const noexcept -> bool {
    return status() == requirements.requiredStatus && security() >= requirements.minimumSecurity &&
        throughput() >= requirements.minimumThroughput;
}

auto HashAlgorithm::matching(const HashRequirements &requirements) -> util::List<HashAlgorithm> {
    auto result = util::List<HashAlgorithm>{};
    for (const auto algorithm : all()) {
        if (algorithm.matches(requirements)) {
            result.append(algorithm);
        }
    }
    return result;
}

auto HashAlgorithm::recommended(const HashRequirements &requirements) noexcept -> std::optional<HashAlgorithm> {
    auto result = std::optional<HashAlgorithm>{};
    for (const auto algorithm : all()) {
        if (!algorithm.matches(requirements)) {
            continue;
        }
        if (!result.has_value() || algorithm.throughput() > result->throughput() ||
            (algorithm.throughput() == result->throughput() && algorithm.security() > result->security())) {
            result = algorithm;
        }
    }
    return result;
}

auto HashAlgorithm::toString() const -> text::String {
    switch (_value) {
    case Sha3_256:
        return "sha3-256"_el;
    case Sha3_384:
        return "sha3-384"_el;
    case Sha3_512:
        return "sha3-512"_el;
    case Sha2_256:
        return "sha-256"_el;
    case Sha2_384:
        return "sha-384"_el;
    case Sha2_512:
        return "sha-512"_el;
    case Sha1:
        return "sha-1"_el;
    case Md5:
        return "md5"_el;
    }
    return {};
}

auto HashAlgorithm::fromString(const text::String &text) noexcept -> std::optional<HashAlgorithm> {
    if (text == "sha3-256"_el) {
        return HashAlgorithm{Sha3_256};
    }
    if (text == "sha3-384"_el) {
        return HashAlgorithm{Sha3_384};
    }
    if (text == "sha3-512"_el) {
        return HashAlgorithm{Sha3_512};
    }
    if (text == "sha-256"_el) {
        return HashAlgorithm{Sha2_256};
    }
    if (text == "sha-384"_el) {
        return HashAlgorithm{Sha2_384};
    }
    if (text == "sha-512"_el) {
        return HashAlgorithm{Sha2_512};
    }
    if (text == "sha-1"_el) {
        return HashAlgorithm{Sha1};
    }
    if (text == "md5"_el) {
        return HashAlgorithm{Md5};
    }
    return {};
}

auto HashAlgorithm::fromStringOrThrow(const text::String &text) -> HashAlgorithm {
    if (const auto result = fromString(text); result.has_value()) {
        return result.value();
    }
    throw err::ParseError{"Unsupported hash algorithm."};
}

auto HashAlgorithm::all() noexcept -> std::span<const HashAlgorithm> {
    static constexpr auto algorithms = std::array<HashAlgorithm, 8>{
        HashAlgorithm{Sha3_256},
        HashAlgorithm{Sha3_384},
        HashAlgorithm{Sha3_512},
        HashAlgorithm{Sha2_256},
        HashAlgorithm{Sha2_384},
        HashAlgorithm{Sha2_512},
        HashAlgorithm{Sha1},
        HashAlgorithm{Md5}};
    return algorithms;
}

auto HashAlgorithm::allSafe() noexcept -> std::span<const HashAlgorithm> {
    static constexpr auto algorithms = std::array<HashAlgorithm, 6>{
        HashAlgorithm{Sha3_256},
        HashAlgorithm{Sha3_384},
        HashAlgorithm{Sha3_512},
        HashAlgorithm{Sha2_256},
        HashAlgorithm{Sha2_384},
        HashAlgorithm{Sha2_512}};
    return algorithms;
}

}
