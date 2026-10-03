// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ApplicationRegExData_fwd.hpp"

#include "../../../re/impl/RegExCache.hpp"

namespace erbsland::core::impl {

/// Lazily created application-scoped regular-expression cache.
/// @tested{RegExCachedTest ApplicationDataAccessorTest}
class ApplicationRegExData final {
public:
    /// Access the synchronized compiled-pattern cache.
    [[nodiscard]] auto cache() noexcept -> re::impl::RegExCache & { return _cache; }

private:
    re::impl::RegExCache _cache; ///< Bounded cache shared by callers in this application scope.
};

}
