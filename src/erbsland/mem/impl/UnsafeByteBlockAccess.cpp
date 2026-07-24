// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "UnsafeByteBlockAccess.hpp"

#include "ByteBlockData.hpp"
#include "Throw.hpp"

#include "../../err/LogicError.hpp"

namespace erbsland::mem::impl {

UnsafeByteBlockAccess::UnsafeByteBlockAccess(const ByteBlock &block) noexcept : _block{&block} {
}
UnsafeByteBlockAccess::UnsafeByteBlockAccess(const ByteBlockEditor &block) noexcept : _reader{&block} {
}
UnsafeByteBlockAccess::UnsafeByteBlockAccess(ByteBlockEditor &block) noexcept : _editor{&block} {
}

auto UnsafeByteBlockAccess::data() const noexcept -> ConstByteSpan {
    if (_block != nullptr) {
        return _block->dataView().dataSpan();
    }
    const auto *editor = _editor != nullptr ? _editor : _reader;
    return editor != nullptr ? editor->dataView().dataSpan() : ConstByteSpan{};
}

auto UnsafeByteBlockAccess::writableData() -> ByteSpan {
    if (_editor == nullptr) {
        throw err::LogicError{"Mutable byte-block access requires a mutable editor"};
    }
    return _editor->dataSpanForWrite();
}

}
