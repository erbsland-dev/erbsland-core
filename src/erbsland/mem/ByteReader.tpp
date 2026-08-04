// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "impl/ByteDataView.hpp"
#include "impl/ByteReadTools.hpp"
#include "impl/Throw.hpp"

#include "../math/SaturatingMath.hpp"
#include "../math/SignedMagnitude.hpp"

#include <limits>

namespace erbsland::mem {

template <impl::NativeByteInteger T>
auto ByteReader::readInteger(const T defaultOnError) noexcept -> T {
    auto result = T{};
    return readIntegerInto(result) ? result : defaultOnError;
}

template <impl::NativeByteInteger T>
auto ByteReader::readIntegerOrThrow() -> T {
    const auto result = impl::ByteReadTools{dataView()}.getIntegerOrThrow<T>(_position, _endianness);
    advance(unit::ByteLength{sizeof(T)});
    return result;
}

template <impl::NativeByteInteger T>
auto ByteReader::readInteger(const ByteIntegerFormat format) -> std::optional<T> {
    try {
        return readIntegerOrThrow<T>(format);
    } catch (...) {
        return std::nullopt;
    }
}

template <impl::NativeByteInteger T>
auto ByteReader::readInteger(const ByteIntegerFormat::Value format) -> std::optional<T> {
    return readInteger<T>(ByteIntegerFormat{format});
}

template <impl::NativeByteInteger T>
auto ByteReader::readIntegerOrThrow(const ByteIntegerFormat format) -> T {
    const auto value = impl::ByteReadTools{dataView()}.getIntegerOrThrow(_position, format, _endianness);
    auto result = T{};
    if (!value.isNegative) {
        if (math::willCastOverflow<T>(value.magnitude)) {
            impl::throwOverflow("Wire integer does not fit the requested type");
        }
        result = static_cast<T>(value.magnitude);
    } else {
        if constexpr (std::unsigned_integral<T>) {
            impl::throwOverflow("A negative wire integer does not fit an unsigned type");
        } else {
            const auto signedMagnitude = math::SignedMagnitude<int64_t>{true, value.magnitude};
            const auto minimum = static_cast<int64_t>(std::numeric_limits<T>::min());
            const auto maximum = static_cast<int64_t>(std::numeric_limits<T>::max());
            if (signedMagnitude.wouldSaturate(minimum, maximum)) {
                impl::throwOverflow("Wire integer does not fit the requested type");
            }
            result = static_cast<T>(signedMagnitude.toSaturatingValue(minimum, maximum));
        }
    }
    advance(value.byteLength);
    return result;
}

template <impl::NativeByteInteger T>
auto ByteReader::readIntegerOrThrow(const ByteIntegerFormat::Value format) -> T {
    return readIntegerOrThrow<T>(ByteIntegerFormat{format});
}

template <impl::NativeByteInteger T>
auto ByteReader::readIntegerInto(T &value) noexcept -> bool {
    if (!impl::ByteReadTools{dataView()}.getIntegerInto(value, _position, _endianness)) {
        return false;
    }
    advance(unit::ByteLength{sizeof(T)});
    return true;
}

}
