// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PasswordHashPolicy.hpp"

namespace erbsland::cryptology {

PasswordHashPolicy::PasswordHashPolicy() noexcept :
    PasswordHashPolicy{PasswordHashAlgorithm::Argon2id, 64U * 1024U, 3U, 4U, 0U, 0U, 0U} {
}

PasswordHashPolicy::PasswordHashPolicy(const unsafe::UnsafeCustomPasswordHashParameters &parameters) noexcept :
    PasswordHashPolicy{
        parameters._algorithm,
        parameters._algorithm == PasswordHashAlgorithm::Argon2id ? parameters._first : 0U,
        parameters._algorithm == PasswordHashAlgorithm::Argon2id ? parameters._second : 0U,
        parameters._algorithm == PasswordHashAlgorithm::Argon2id ? parameters._third : 0U,
        parameters._large,
        parameters._algorithm == PasswordHashAlgorithm::Scrypt ? parameters._first : 0U,
        parameters._algorithm == PasswordHashAlgorithm::Scrypt ? parameters._second : 0U} {
}

PasswordHashPolicy::PasswordHashPolicy(
    const PasswordHashAlgorithm algorithm,
    const uint32_t memoryKiB,
    const uint32_t passes,
    const uint32_t lanes,
    const uint64_t scryptCost,
    const uint32_t scryptBlockSize,
    const uint32_t scryptParallelization) noexcept :
    _algorithm{algorithm},
    _memoryKiB{memoryKiB},
    _passes{passes},
    _lanes{lanes},
    _scryptCost{scryptCost},
    _scryptBlockSize{scryptBlockSize},
    _scryptParallelization{scryptParallelization} {
}

auto PasswordHashPolicy::recommended() noexcept -> PasswordHashPolicy {
    return {};
}

auto PasswordHashPolicy::lowMemory() noexcept -> PasswordHashPolicy {
    return {PasswordHashAlgorithm::Argon2id, 19U * 1024U, 2U, 1U, 0U, 0U, 0U};
}

auto PasswordHashPolicy::scrypt() noexcept -> PasswordHashPolicy {
    return {PasswordHashAlgorithm::Scrypt, 0U, 0U, 0U, uint64_t{1U} << 17U, 8U, 1U};
}

auto PasswordHashPolicy::isEqualTo(const PasswordHashPolicy &other) const noexcept -> bool {
    return _algorithm == other._algorithm && _memoryKiB == other._memoryKiB && _passes == other._passes &&
        _lanes == other._lanes && _scryptCost == other._scryptCost && _scryptBlockSize == other._scryptBlockSize &&
        _scryptParallelization == other._scryptParallelization;
}

}
