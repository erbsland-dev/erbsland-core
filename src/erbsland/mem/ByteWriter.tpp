// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ByteArray.hpp"

#include "impl/ByteWriterTools.hpp"

#include "../math/IntegerMath.hpp"

namespace erbsland::mem {

template <impl::NativeByteInteger T>
auto ByteWriter::writeInteger(const T value) -> ByteWriter & {
    auto bytes = ByteArray<sizeof(T)>{};
    bytes.setIntegerOrThrow(unit::ByteIndex::zero(), value, _endianness);
    return writeBytes(bytes.span());
}

template <impl::NativeByteInteger T>
auto ByteWriter::writeIntegerOrThrow(const T value, const ByteIntegerFormat format) -> ByteWriter & {
    const auto isNegative = math::isNegativeValue(value);
    const auto magnitude = static_cast<uint64_t>(math::toUnsignedAbsolute(value));
    impl::ByteWriterTools{*this}.writeIntegerOrThrow(isNegative, magnitude, format);
    return *this;
}

}
