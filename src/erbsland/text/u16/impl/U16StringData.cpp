// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U16StringData.hpp"

#include <cstring>
#include <exception>
#include <limits>

namespace erbsland::text::impl {

auto createU16StringData(const std::u16string_view stdString) -> U16StringDataPtr {
    if (stdString.empty()) {
        return U16StringDataPtr{};
    }
    if (stdString.size() == std::numeric_limits<std::size_t>::max() ||
        !U16StringData::canAllocateWithCapacity(stdString.size() + 1U)) {
        std::terminate();
    }
    const auto size = static_cast<U16StringData::SizeType>(stdString.size()) + 1U;
    auto *data = U16StringData::create(size, size);
    std::memcpy(data->data(), stdString.data(), stdString.size() * sizeof(char16_t));
    data->data()[stdString.size()] = u'\0';
    return U16StringDataPtr{data};
}

auto createU16StringData(const std::size_t actualStringDataSize) -> U16StringDataPtr {
    return createU16StringData(actualStringDataSize, actualStringDataSize);
}

auto createU16StringData(const std::size_t actualStringDataSize, const std::size_t reservedCapacity)
    -> U16StringDataPtr {
    if (actualStringDataSize > reservedCapacity) {
        std::terminate();
    }
    if (reservedCapacity == 0) {
        return U16StringDataPtr{};
    }
    if (reservedCapacity == std::numeric_limits<std::size_t>::max() ||
        !U16StringData::canAllocateWithCapacity(reservedCapacity + 1U)) {
        std::terminate();
    }
    const auto size = static_cast<U16StringData::SizeType>(actualStringDataSize) + 1U;
    const auto capacity = static_cast<U16StringData::SizeType>(reservedCapacity) + 1U;
    auto *data = U16StringData::create(size, capacity);
    data->data()[actualStringDataSize] = u'\0';
    return U16StringDataPtr{data};
}

}
