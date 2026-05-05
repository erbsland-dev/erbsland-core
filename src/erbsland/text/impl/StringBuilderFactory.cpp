// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "StringBuilderFactory.hpp"

#include "../u16/impl/U16StringBuilder.hpp"
#include "../u32/impl/U32StringBuilder.hpp"
#include "../u8/impl/U8StringBuilder.hpp"

#include "../../math/SaturatingMath.hpp"

namespace erbsland::text::impl {

auto createStringBuilder(const StringKind kind) -> mem::SharedDataPointer<StringBuilderBase> {
    switch (kind) {
    case StringKind::U8:
        return mem::SharedDataPointer<StringBuilderBase>{new U8StringBuilder{}};
    case StringKind::U16:
        return mem::SharedDataPointer<StringBuilderBase>{new U16StringBuilder{}};
    case StringKind::U32:
        return mem::SharedDataPointer<StringBuilderBase>{new U32StringBuilder{}};
    }
    return mem::SharedDataPointer<StringBuilderBase>{new U8StringBuilder{}};
}

auto createStringBuilder(const StringKind kind, const unit::CpLength capacity)
    -> mem::SharedDataPointer<StringBuilderBase> {
    switch (kind) {
    case StringKind::U8: {
        using Value = unit::ByteLength::Value;
        const auto rawCapacity = math::saturatingMultiply(static_cast<Value>(capacity.toRawValue()), Value{4U});
        return createU8StringBuilder(unit::ByteLength{rawCapacity});
    }
    case StringKind::U16: {
        using Value = unit::U16DataLength::Value;
        const auto rawCapacity = math::saturatingMultiply(static_cast<Value>(capacity.toRawValue()), Value{2U});
        const auto boundedCapacity =
            rawCapacity > unit::U16DataLength::cRawMaximum ? unit::U16DataLength::cRawMaximum : rawCapacity;
        return createU16StringBuilder(unit::U16DataLength{boundedCapacity});
    }
    case StringKind::U32:
        return createU32StringBuilder(capacity);
    }
    return createU8StringBuilder(unit::ByteLength{});
}

auto createU8StringBuilder(const unit::ByteLength capacity) -> mem::SharedDataPointer<StringBuilderBase> {
    return mem::SharedDataPointer<StringBuilderBase>{new U8StringBuilder{capacity}};
}

auto createU16StringBuilder(const unit::U16DataLength capacity) -> mem::SharedDataPointer<StringBuilderBase> {
    return mem::SharedDataPointer<StringBuilderBase>{new U16StringBuilder{capacity}};
}

auto createU32StringBuilder(const unit::CpLength capacity) -> mem::SharedDataPointer<StringBuilderBase> {
    return mem::SharedDataPointer<StringBuilderBase>{new U32StringBuilder{capacity}};
}

}
