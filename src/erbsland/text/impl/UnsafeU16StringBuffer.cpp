// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "UnsafeU16StringBuffer.hpp"

#include "../StringConverter.hpp"
#include "../u16/impl/U16Encoding.hpp"
#include "../u16/impl/U16StringEncodingTools.hpp"
#include "../u8/impl/U8Writer.hpp"
#include "../u8/U8StringEditor.hpp"

namespace erbsland::text::impl {

using unit::U16DataLength;
using unit::U16DataRange;

UnsafeU16StringBuffer::UnsafeU16StringBuffer(std::size_t dataSize) : _data{createDataForDataSize(dataSize)} {
}

UnsafeU16StringBuffer::UnsafeU16StringBuffer(U16DataLength capacity) :
    UnsafeU16StringBuffer{dataSizeForCapacity(capacity)} {
}

auto UnsafeU16StringBuffer::data() noexcept -> char16_t * {
    if (_data.isNull()) {
        return nullptr;
    }
    return _data.get()->data();
}

auto UnsafeU16StringBuffer::data() const noexcept -> const char16_t * {
    if (_data.isNull()) {
        return nullptr;
    }
    return _data.constGet()->data();
}

#ifdef ERBSLAND_WCHAR_16BIT
auto UnsafeU16StringBuffer::dataAsWide() noexcept -> wchar_t * {
    if (_data.isNull()) {
        return nullptr;
    }
    return reinterpret_cast<wchar_t *>(_data.get()->data());
}

auto UnsafeU16StringBuffer::dataAsWide() const noexcept -> const wchar_t * {
    if (_data.isNull()) {
        return nullptr;
    }
    return reinterpret_cast<const wchar_t *>(_data.constGet()->data());
}
#endif

auto UnsafeU16StringBuffer::dataSize() const noexcept -> std::size_t {
    if (_data.isNull()) {
        return 0;
    }
    return _data.constGet()->capacity();
}

auto UnsafeU16StringBuffer::capacity() const noexcept -> U16DataLength {
    const auto size = dataSize();
    if (size == 0U) {
        return U16DataLength::zero();
    }
    return U16DataLength::fromSizeT(size - 1U);
}

auto UnsafeU16StringBuffer::take(const U16DataLength length) -> U16StringEditor {
    const auto finalLength = checkedFinalLength(length);
    if (finalLength.isZero()) {
        _data.reset();
        return {};
    }
    auto data = std::move(_data);
    const auto size = finalLength.toSizeT();
    data.get()->setSize(static_cast<U16StringData::SizeType>(size + 1U));
    data.get()->data()[size] = u'\0';
    return U16StringEditor{U16StringSharedStorage{std::move(data), U16DataRange::fromSizeT(size)}};
}

auto UnsafeU16StringBuffer::take(const std::size_t length) -> U16StringEditor {
    const auto finalLength = U16DataLength::fromSizeT(length);
    return take(finalLength);
}

auto UnsafeU16StringBuffer::takeAsUtf8(const U16DataLength length) -> U8StringEditor {
    const auto u16String = take(length);
    return U8StringEditor{StringConverter{u16String}.toU8String()};
}

auto UnsafeU16StringBuffer::takeAsUtf8(std::size_t length) -> U8StringEditor {
    return takeAsUtf8(U16DataLength::fromSizeT(length));
}

auto UnsafeU16StringBuffer::createDataForDataSize(std::size_t dataSize) -> U16StringDataPtr {
    if (dataSize == 0U) {
        return {};
    }
    if (!U16StringData::canAllocateWithCapacity(dataSize)) {
        std::terminate();
    }
    const auto size = static_cast<U16StringData::SizeType>(dataSize);
    auto *data = U16StringData::create(size, size);
    data->data()[dataSize - 1U] = u'\0';
    return U16StringDataPtr{data};
}

auto UnsafeU16StringBuffer::dataSizeForCapacity(U16DataLength capacity) -> std::size_t {
    if (capacity.isInfinite()) {
        std::terminate();
    }
    const auto size = capacity.toSizeTOrThrow();
    if (size == std::numeric_limits<std::size_t>::max()) {
        std::terminate();
    }
    return size + 1U;
}

auto UnsafeU16StringBuffer::checkedFinalLength(U16DataLength length) const -> U16DataLength {
    if (length.isInfinite()) {
        return capacity();
    }
    if (length > capacity()) {
        std::terminate();
    }
    return length;
}

}
