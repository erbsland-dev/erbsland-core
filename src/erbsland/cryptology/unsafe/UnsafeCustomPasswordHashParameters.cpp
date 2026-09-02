// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "UnsafeCustomPasswordHashParameters.hpp"

#include "../../err/ParameterError.hpp"
#include "../../text/Literals.hpp"

#include <limits>

namespace erbsland::cryptology::unsafe {

using namespace text::literals;

UnsafeCustomPasswordHashParameters::UnsafeCustomPasswordHashParameters(
    const PasswordHashAlgorithm algorithm,
    const uint32_t first,
    const uint32_t second,
    const uint32_t third,
    const uint64_t large) noexcept :
    _algorithm{algorithm}, _first{first}, _second{second}, _third{third}, _large{large} {
}

auto UnsafeCustomPasswordHashParameters::argon2id(const uint32_t memoryKiB, const uint32_t passes, const uint32_t lanes)
    -> UnsafeCustomPasswordHashParameters {
    if (!areValidArgon2idCosts(memoryKiB, passes, lanes)) {
        throw err::ParameterError{"Argon2id costs exceed the supported safety bounds"_el, "parameters"_el};
    }
    return {PasswordHashAlgorithm::Argon2id, memoryKiB, passes, lanes, 0U};
}

auto UnsafeCustomPasswordHashParameters::scrypt(
    const uint64_t cost, const uint32_t blockSize, const uint32_t parallelization)
    -> UnsafeCustomPasswordHashParameters {
    if (!areValidScryptCosts(cost, blockSize, parallelization)) {
        throw err::ParameterError{"scrypt costs exceed the supported safety bounds"_el, "parameters"_el};
    }
    return {PasswordHashAlgorithm::Scrypt, blockSize, parallelization, 0U, cost};
}

auto UnsafeCustomPasswordHashParameters::areValidArgon2idCosts(
    const uint32_t memoryKiB, const uint32_t passes, const uint32_t lanes) noexcept -> bool {
    return lanes != 0U && lanes <= 16U && passes != 0U && passes <= 10U && memoryKiB >= 8U * lanes &&
        memoryKiB <= 1024U * 1024U;
}

auto UnsafeCustomPasswordHashParameters::areValidScryptCosts(
    const uint64_t cost, const uint32_t blockSize, const uint32_t parallelization) noexcept -> bool {
    if (cost <= 1U || (cost & (cost - 1U)) != 0U || blockSize == 0U || parallelization == 0U ||
        blockSize > std::numeric_limits<uint32_t>::max() / 128U) {
        return false;
    }
    constexpr auto maximumUnits = uint64_t{1024U} * 1024U;
    return cost <= maximumUnits / blockSize && parallelization <= 16U &&
        cost * blockSize <= (maximumUnits * 10U) / parallelization;
}

}
