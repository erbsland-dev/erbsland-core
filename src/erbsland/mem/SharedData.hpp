// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ReferenceCounter.hpp"

#include "impl/SharedDataPointerTraits_fwd.hpp"

namespace erbsland::mem {

/// The base class for custom implicitly/explicitly shared data.
/// @seedoc{/reference/mem/memory_and_byte_data}
/// @warning This is an advanced data type, meant for people extending the library.
/// Do not use it unless you understand the implications and have a specific need.
class SharedData {
    template <typename, typename>
    friend struct impl::SharedDataPointerTraits;
    template <typename>
    friend struct impl::RegularSharedDataPointerTraits;
    template <typename>
    friend struct impl::PolymorphicSharedDataPointerTraits;

public:
    /// Create a shared-data base with a zero reference count.
    SharedData() = default;
    /// Copy shared-data state without copying its reference counter.
    /// @param other The source shared-data base.
    SharedData([[maybe_unused]] const SharedData &other) noexcept {}
    /// Move shared-data state without moving its reference counter.
    /// @param other The source shared-data base.
    SharedData([[maybe_unused]] SharedData &&other) noexcept {}
    /// Assign shared-data state without assigning its reference counter.
    /// @param other The source shared-data base.
    auto operator=([[maybe_unused]] const SharedData &other) noexcept -> SharedData & { return *this; }
    /// Move-assign shared-data state without assigning its reference counter.
    /// @param other The source shared-data base.
    auto operator=([[maybe_unused]] SharedData &&other) noexcept -> SharedData & { return *this; }

    // defaults
    ~SharedData() = default;

private:
    ReferenceCounter _referenceCount; ///< The reference counter.
};

}
