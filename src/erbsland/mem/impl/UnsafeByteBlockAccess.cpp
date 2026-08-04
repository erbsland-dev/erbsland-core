// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "UnsafeByteBlockAccess.hpp"

namespace erbsland::mem::impl {

UnsafeByteBlockAccess::UnsafeByteBlockAccess(const ByteBlock &block) noexcept : _block{block} {
}

auto UnsafeByteBlockAccess::dataView() const noexcept -> ByteDataView {
    return _block.dataView();
}

}
