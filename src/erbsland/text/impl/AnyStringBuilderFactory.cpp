// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "AnyStringBuilderFactory.hpp"

#include "../u16/impl/U16StringBuilder.hpp"
#include "../u32/impl/U32StringBuilder.hpp"
#include "../u8/impl/U8StringBuilder.hpp"

#include "../../math/SaturatingMath.hpp"

namespace erbsland::text::impl {

using mem::SharedDataPointer;
using unit::ByteCount;
using unit::CpCount;
using unit::U16Count;

auto createAnyStringBuilder(const StringKind kind) -> SharedDataPointer<AnyStringBuilderBase> {
    switch (kind) {
    case StringKind::U8:
        return SharedDataPointer<AnyStringBuilderBase>{new U8StringBuilder{}};
    case StringKind::U16:
        return SharedDataPointer<AnyStringBuilderBase>{new U16StringBuilder{}};
    case StringKind::U32:
        return SharedDataPointer<AnyStringBuilderBase>{new U32StringBuilder{}};
    }
    return SharedDataPointer<AnyStringBuilderBase>{new U8StringBuilder{}};
}

auto createAnyStringBuilder(const StringKind kind, const CpCount capacity) -> SharedDataPointer<AnyStringBuilderBase> {
    switch (kind) {
    case StringKind::U8: {
        using Value = ByteCount::Value;
        const auto rawCapacity = math::saturatingMultiply(static_cast<Value>(capacity.toRawValue()), Value{4U});
        return createU8StringBuilder(ByteCount::fromInteger(rawCapacity));
    }
    case StringKind::U16: {
        using Value = U16Count::Value;
        const auto rawCapacity = math::saturatingMultiply(static_cast<Value>(capacity.toRawValue()), Value{2U});
        const auto boundedCapacity = rawCapacity > U16Count::cRawMaximum ? U16Count::cRawMaximum : rawCapacity;
        return createU16StringBuilder(U16Count{boundedCapacity, U16Count::Unchecked{}});
    }
    case StringKind::U32:
        return createU32StringBuilder(capacity);
    }
    return createU8StringBuilder(ByteCount{});
}

auto createU8StringBuilder(const ByteCount capacity, const bool sensitive) -> SharedDataPointer<AnyStringBuilderBase> {
    return SharedDataPointer<AnyStringBuilderBase>{new U8StringBuilder{capacity, sensitive}};
}

auto createU16StringBuilder(const U16Count capacity) -> SharedDataPointer<AnyStringBuilderBase> {
    return SharedDataPointer<AnyStringBuilderBase>{new U16StringBuilder{capacity}};
}

auto createU32StringBuilder(const CpCount capacity) -> SharedDataPointer<AnyStringBuilderBase> {
    return SharedDataPointer<AnyStringBuilderBase>{new U32StringBuilder{capacity}};
}

}
