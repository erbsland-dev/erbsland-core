// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ReferenceCounter.hpp"

#include "impl/SharedDataPointerTraits_fwd.hpp"

namespace erbsland::mem {

/// The base class for custom implicitly/explicitly shared data.
/// @seedoc{/reference/mem/cow_storage}
/// @warning This is an advanced data type, meant for people extending the library.
/// Do not use it unless you understand the implications and have a specific need.
class SharedData {
public:
    /// Create a shared-data base with a zero reference count.
    SharedData() = default;

    /// Copy shared-data state without copying its reference counter.
    /// @param other The source shared-data base.
    SharedData(const SharedData &) noexcept {}
    /// Move shared-data state without moving its reference counter.
    /// @param other The source shared-data base.
    SharedData(SharedData &&) noexcept {}
    // defaults
    ~SharedData() = default;
    /// Assign shared-data state without assigning its reference counter.
    /// @param other The source shared-data base.
    auto operator=(const SharedData &) noexcept -> SharedData & { return *this; }
    /// Move-assign shared-data state without assigning its reference counter.
    /// @param other The source shared-data base.
    auto operator=(SharedData &&) noexcept -> SharedData & { return *this; }

private:
    template <typename, typename>
    friend struct impl::SharedDataPointerTraits;
    template <typename>
    friend struct impl::RegularSharedDataPointerTraits;
    template <typename>
    friend struct impl::PolymorphicSharedDataPointerTraits;

private:
    ReferenceCounter _referenceCount;
};

}
