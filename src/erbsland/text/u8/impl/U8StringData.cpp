// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U8StringData.hpp"

#include "../../../mem/UnsafeCharPtr.hpp"

#include <cstring>
#include <exception>
#include <limits>

namespace erbsland::text::impl {

auto createU8StringData(const std::string_view stdString) -> U8StringDataPtr {
    if (stdString.empty()) {
        return U8StringDataPtr{};
    }
    if (stdString.size() == std::numeric_limits<std::size_t>::max() ||
        !U8StringData::canAllocateWithCapacity(stdString.size() + 1U)) {
        std::terminate();
    }
    const auto size = static_cast<U8StringData::SizeType>(stdString.size()) + 1U;
    auto *data = U8StringData::create(size, size);
    std::memcpy(data->data(), stdString.data(), stdString.size());
    data->data()[stdString.size()] = '\0';
    return U8StringDataPtr{data};
}

auto createU8StringData(const std::u8string_view stdString) -> U8StringDataPtr {
    return createU8StringData(
        std::string_view{reinterpret_cast<mem::UnsafeConstCharPtr>(stdString.data()), stdString.size()});
}

auto createU8StringData(const std::size_t actualStringDataSize) -> U8StringDataPtr {
    return createU8StringData(actualStringDataSize, actualStringDataSize);
}

auto createU8StringData(const std::size_t actualStringDataSize, const std::size_t reservedCapacity) -> U8StringDataPtr {
    if (actualStringDataSize > reservedCapacity) {
        std::terminate();
    }
    if (reservedCapacity == 0) {
        return U8StringDataPtr{};
    }
    if (reservedCapacity == std::numeric_limits<std::size_t>::max() ||
        !U8StringData::canAllocateWithCapacity(reservedCapacity + 1U)) {
        std::terminate();
    }
    const auto size = static_cast<U8StringData::SizeType>(actualStringDataSize) + 1U;
    const auto capacity = static_cast<U8StringData::SizeType>(reservedCapacity) + 1U;
    auto *data = U8StringData::create(size, capacity);
    data->data()[actualStringDataSize] = '\0';
    return U8StringDataPtr{data};
}

}
