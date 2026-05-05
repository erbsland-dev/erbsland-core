// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U32StringData.hpp"

#include <cstring>
#include <exception>
#include <limits>

namespace erbsland::text::impl {

auto createU32StringData(const std::u32string_view stdString) -> U32StringDataPtr {
    if (stdString.empty()) {
        return U32StringDataPtr{};
    }
    if (stdString.size() == std::numeric_limits<std::size_t>::max() ||
        !U32StringData::canAllocateWithCapacity(stdString.size() + 1U)) {
        std::terminate();
    }
    const auto size = static_cast<U32StringData::SizeType>(stdString.size()) + 1U;
    auto *data = U32StringData::create(size, size);
    std::memcpy(data->data(), stdString.data(), stdString.size() * sizeof(char32_t));
    data->data()[stdString.size()] = U'\0';
    return U32StringDataPtr{data};
}

auto createU32StringData(const std::size_t actualStringDataSize) -> U32StringDataPtr {
    return createU32StringData(actualStringDataSize, actualStringDataSize);
}

auto createU32StringData(const std::size_t actualStringDataSize, const std::size_t reservedCapacity)
    -> U32StringDataPtr {
    if (actualStringDataSize > reservedCapacity) {
        std::terminate();
    }
    if (reservedCapacity == 0) {
        return U32StringDataPtr{};
    }
    if (reservedCapacity == std::numeric_limits<std::size_t>::max() ||
        !U32StringData::canAllocateWithCapacity(reservedCapacity + 1U)) {
        std::terminate();
    }
    const auto size = static_cast<U32StringData::SizeType>(actualStringDataSize) + 1U;
    const auto capacity = static_cast<U32StringData::SizeType>(reservedCapacity) + 1U;
    auto *data = U32StringData::create(size, capacity);
    data->data()[actualStringDataSize] = U'\0';
    return U32StringDataPtr{data};
}

}
