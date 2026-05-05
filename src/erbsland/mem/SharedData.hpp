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
    SharedData() = default;
    SharedData(const SharedData &) noexcept {}
    SharedData(SharedData &&) noexcept {}
    ~SharedData() = default;
    auto operator=(const SharedData &) noexcept -> SharedData & { return *this; }
    auto operator=(SharedData &&) noexcept -> SharedData & { return *this; }

private:
    template <typename, typename>
    friend struct impl::SharedDataPointerTraits;

private:
    ReferenceCounter _referenceCount;
};

}
