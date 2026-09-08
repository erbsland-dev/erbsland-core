// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ApplicationRandomData_fwd.hpp"

#include "../../../random/Random_fwd.hpp"

#include <functional>
#include <mutex>

namespace erbsland::core::impl {

/// Lazily initialized random generators shared by an application.
/// @tested{RandomApplicationTest}
class ApplicationRandomData final {
public:
    /// Factory for a random generator.
    using Factory = std::function<random::RandomPtr()>;

public:
    /// Access the regular random generator, creating it with `factory` on first use.
    [[nodiscard]] auto random(const Factory &factory) -> random::Random &;
    /// Access the secure random generator, creating it with `factory` on first use.
    [[nodiscard]] auto secureRandom(const Factory &factory) -> random::Random &;

private:
    std::mutex _mutex;               ///< Serializes generator creation.
    random::RandomPtr _random;       ///< Regular random generator.
    random::RandomPtr _secureRandom; ///< Cryptographically secure random generator.
};

}
