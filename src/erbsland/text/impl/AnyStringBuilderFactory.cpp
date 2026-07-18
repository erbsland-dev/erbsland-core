// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "AnyStringBuilderFactory.hpp"

#include "../u16/impl/U16StringBuilder.hpp"
#include "../u32/impl/U32StringBuilder.hpp"
#include "../u8/impl/U8StringBuilder.hpp"

#include "../../math/SaturatingMath.hpp"

namespace erbsland::text::impl {

using mem::SharedDataPointer;
using unit::ByteLength;
using unit::CpLength;
using unit::U16DataLength;

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

auto createAnyStringBuilder(const StringKind kind, const CpLength capacity) -> SharedDataPointer<AnyStringBuilderBase> {
    switch (kind) {
    case StringKind::U8: {
        using Value = ByteLength::Value;
        const auto rawCapacity = math::saturatingMultiply(static_cast<Value>(capacity.toRawValue()), Value{4U});
        return createU8StringBuilder(ByteLength{rawCapacity});
    }
    case StringKind::U16: {
        using Value = U16DataLength::Value;
        const auto rawCapacity = math::saturatingMultiply(static_cast<Value>(capacity.toRawValue()), Value{2U});
        const auto boundedCapacity =
            rawCapacity > U16DataLength::cRawMaximum ? U16DataLength::cRawMaximum : rawCapacity;
        return createU16StringBuilder(U16DataLength{boundedCapacity});
    }
    case StringKind::U32:
        return createU32StringBuilder(capacity);
    }
    return createU8StringBuilder(ByteLength{});
}

auto createU8StringBuilder(const ByteLength capacity) -> SharedDataPointer<AnyStringBuilderBase> {
    return SharedDataPointer<AnyStringBuilderBase>{new U8StringBuilder{capacity}};
}

auto createU16StringBuilder(const U16DataLength capacity) -> SharedDataPointer<AnyStringBuilderBase> {
    return SharedDataPointer<AnyStringBuilderBase>{new U16StringBuilder{capacity}};
}

auto createU32StringBuilder(const CpLength capacity) -> SharedDataPointer<AnyStringBuilderBase> {
    return SharedDataPointer<AnyStringBuilderBase>{new U32StringBuilder{capacity}};
}

}
