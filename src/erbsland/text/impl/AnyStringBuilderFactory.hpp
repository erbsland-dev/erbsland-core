// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "AnyStringBuilderBase.hpp"

#include "../StringKind.hpp"

#include "../../mem/SharedDataPointer.hpp"
#include "../../unit/ByteCount.hpp"
#include "../../unit/CpCount.hpp"
#include "../../unit/U16Count.hpp"

namespace erbsland::text::impl {

/// Create an empty builder for a selected string representation.
[[nodiscard]] auto createAnyStringBuilder(StringKind kind) -> mem::SharedDataPointer<AnyStringBuilderBase>;
/// Create a builder with capacity expressed in code points.
[[nodiscard]] auto createAnyStringBuilder(StringKind kind, unit::CpCount capacity)
    -> mem::SharedDataPointer<AnyStringBuilderBase>;
/// Create a UTF-8 builder with byte capacity and optional sensitive storage.
[[nodiscard]] auto createU8StringBuilder(unit::ByteCount capacity, bool sensitive = false)
    -> mem::SharedDataPointer<AnyStringBuilderBase>;
/// Create a UTF-16 builder with code-unit capacity.
[[nodiscard]] auto createU16StringBuilder(unit::U16Count capacity) -> mem::SharedDataPointer<AnyStringBuilderBase>;
/// Create a UTF-32 builder with code-point capacity.
[[nodiscard]] auto createU32StringBuilder(unit::CpCount capacity) -> mem::SharedDataPointer<AnyStringBuilderBase>;

}
