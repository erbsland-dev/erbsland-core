// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "SharedData.hpp"

namespace erbsland::mem {

/// Base class for polymorphic shared data.
///
/// Derive from this class when a `SharedDataPointer` shall manage an abstract base type and detach by virtual clone.
/// Implementations must return a newly allocated copy of the same dynamic type from `clone()`.
/// @seedoc{/reference/mem/memory_and_byte_data}
/// @tested{SharedDataTest}
class SharedVirtualData : public SharedData {
public:
    // defaults
    SharedVirtualData() = default;
    SharedVirtualData(const SharedVirtualData &) noexcept = default;
    SharedVirtualData(SharedVirtualData &&) noexcept = default;
    auto operator=(const SharedVirtualData &) noexcept -> SharedVirtualData & = default;
    auto operator=(SharedVirtualData &&) noexcept -> SharedVirtualData & = default;

    /// Destroy this virtual shared data.
    virtual ~SharedVirtualData() = default;

public:
    /// Create an unreferenced polymorphic copy of this data.
    [[nodiscard]] virtual auto clone() const -> SharedVirtualData * = 0;
};

}
