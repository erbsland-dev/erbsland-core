// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ApplicationRandomData_fwd.hpp"

#include "../../../random/Random_fwd.hpp"

namespace erbsland::core::impl {

/// Random generators shared by an application.
/// @tested{RandomApplicationTest}
class ApplicationRandomData final {
public:
    // defaults
    ApplicationRandomData();
    ~ApplicationRandomData();

public:
    /// Access the regular random generator.
    [[nodiscard]] auto random() noexcept -> random::Random &;
    /// Access the secure random generator.
    [[nodiscard]] auto secureRandom() noexcept -> random::Random &;

#ifdef ERBSLAND_CORE_DEVELOPER_BUILD
    /// Replace the regular random generator, restoring the default generator for a null pointer.
    /// This method must only be called during application construction, before the generator becomes accessible.
    void setRandom(random::RandomPtr random);
    /// Replace the secure random generator, restoring the default generator for a null pointer.
    /// This method must only be called during application construction, before the generator becomes accessible.
    void setSecureRandom(random::RandomPtr secureRandom);
#endif

private:
    random::RandomPtr _random;       ///< Regular random generator.
    random::RandomPtr _secureRandom; ///< Cryptographically secure random generator.
};

}
