// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "StringBuilderBase.hpp"

#include "../StringKind.hpp"

#include "../../mem/SharedDataPointer.hpp"
#include "../../unit/ByteLength.hpp"
#include "../../unit/CpLength.hpp"
#include "../../unit/U16DataLength.hpp"

namespace erbsland::text::impl {

[[nodiscard]] auto createStringBuilder(StringKind kind) -> mem::SharedDataPointer<StringBuilderBase>;
[[nodiscard]] auto createStringBuilder(StringKind kind, unit::CpLength capacity)
    -> mem::SharedDataPointer<StringBuilderBase>;
[[nodiscard]] auto createU8StringBuilder(unit::ByteLength capacity) -> mem::SharedDataPointer<StringBuilderBase>;
[[nodiscard]] auto createU16StringBuilder(unit::U16DataLength capacity) -> mem::SharedDataPointer<StringBuilderBase>;
[[nodiscard]] auto createU32StringBuilder(unit::CpLength capacity) -> mem::SharedDataPointer<StringBuilderBase>;

}
