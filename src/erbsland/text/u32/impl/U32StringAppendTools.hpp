// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U32StringDataView.hpp"
#include "U32StringSharedStorage.hpp"

#include "../../../unit/CpLength.hpp"
#include "../../../unit/ElementCount.hpp"
#include "../../Char.hpp"

namespace erbsland::text::impl {

/// Append algorithms for `U32String`.
/// @tested{U32StringTest}
class U32StringAppendTools final {
public:
    explicit U32StringAppendTools(U32StringSharedStorage &storage) noexcept : _storage{storage} {}

public:
    /// Append UTF-32 bytes from a data view.
    auto append(const U32StringDataView &text) -> U32StringSharedStorage &;
    /// Append UTF-32 bytes from a data view multiple times.
    auto append(const U32StringDataView &text, unit::ElementCount count) -> U32StringSharedStorage &;
    /// Append one Unicode code point.
    auto append(Char character) -> U32StringSharedStorage &;
    /// Append one Unicode code point multiple times.
    auto append(Char character, unit::CpLength count) -> U32StringSharedStorage &;

private:
    U32StringSharedStorage &_storage;
};

}
