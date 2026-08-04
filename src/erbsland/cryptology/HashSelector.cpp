// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "HashSelector.hpp"

#include "../core/Application.hpp"
#include "../util/List.hpp"

#include <algorithm>

namespace erbsland::cryptology {

auto HashSelector::isSafe(const HashAlgorithm algorithm) const -> bool {
    const auto configuration = core::application().cryptologyConfiguration().snapshot();
    return effectiveStatus(algorithm, configuration) == CryptographicStatus::Acceptable &&
        algorithm.security() >= CryptographicSecurity::Standard;
}

auto HashSelector::matches(const HashAlgorithm algorithm) const -> bool {
    return matches(algorithm, core::application().cryptologyConfiguration().snapshot());
}

auto HashSelector::status(const HashAlgorithm algorithm) const -> CryptographicStatus {
    return effectiveStatus(algorithm, core::application().cryptologyConfiguration().snapshot());
}

auto HashSelector::allAccepted() const -> util::List<HashAlgorithm> {
    const auto configuration = core::application().cryptologyConfiguration().snapshot();
    auto result = util::List<HashAlgorithm>{};
    for (const auto algorithm : HashAlgorithm::all()) {
        if (effectiveStatus(algorithm, configuration) == CryptographicStatus::Acceptable &&
            algorithm.security() >= CryptographicSecurity::Standard) {
            result.append(algorithm);
        }
    }
    return result;
}

auto HashSelector::matching() const -> util::List<HashAlgorithm> {
    const auto configuration = core::application().cryptologyConfiguration().snapshot();
    auto result = util::List<HashAlgorithm>{};
    for (const auto algorithm : HashAlgorithm::all()) {
        if (matches(algorithm, configuration)) {
            result.append(algorithm);
        }
    }
    return result;
}

auto HashSelector::recommended() const -> std::optional<HashAlgorithm> {
    const auto configuration = core::application().cryptologyConfiguration().snapshot();
    auto result = std::optional<HashAlgorithm>{};
    for (const auto algorithm : HashAlgorithm::all()) {
        if (!matches(algorithm, configuration)) {
            continue;
        }
        if (!result.has_value() || algorithm.throughput() > result->throughput() ||
            (algorithm.throughput() == result->throughput() && algorithm.security() > result->security())) {
            result = algorithm;
        }
    }
    return result;
}

auto HashSelector::isValid(const HashAlgorithm algorithm) noexcept -> bool {
    return static_cast<uint8_t>(algorithm.toRawValue()) <= static_cast<uint8_t>(HashAlgorithm::Md5);
}

auto HashSelector::libraryStatus(const HashAlgorithm algorithm) noexcept -> CryptographicStatus {
    switch (algorithm.toRawValue()) {
    case HashAlgorithm::Sha3_256:
    case HashAlgorithm::Sha3_384:
    case HashAlgorithm::Sha3_512:
    case HashAlgorithm::Sha2_256:
    case HashAlgorithm::Sha2_384:
    case HashAlgorithm::Sha2_512:
        return CryptographicStatus::Acceptable;
    case HashAlgorithm::Sha1:
    case HashAlgorithm::Md5:
        return CryptographicStatus::Disallowed;
    }
    return CryptographicStatus::Disallowed;
}

auto HashSelector::effectiveStatus(
    const HashAlgorithm algorithm, const impl::CryptologyConfigurationSnapshot &configuration) noexcept
    -> CryptographicStatus {
    const auto baseStatus = libraryStatus(algorithm);
    const auto maximumStatus = configuration.maximumStatus(algorithm);
    return maximumStatus.has_value() ? std::min(baseStatus, maximumStatus.value()) : baseStatus;
}

auto HashSelector::matches(
    const HashAlgorithm algorithm, const impl::CryptologyConfigurationSnapshot &configuration) const noexcept -> bool {
    return isValid(algorithm) && effectiveStatus(algorithm, configuration) == _requirements.requiredStatus &&
        algorithm.security() >= _requirements.minimumSecurity &&
        algorithm.throughput() >= _requirements.minimumThroughput;
}

}
