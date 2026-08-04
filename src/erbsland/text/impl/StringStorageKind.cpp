// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "StringStorageKind.hpp"

#include "../Literals.hpp"

namespace erbsland::text::impl {

using namespace text::literals;

auto storageKind([[maybe_unused]] const U8StringSharedStorage &storage) noexcept -> StringStorageKind {
    return StringStorageKind::Shared;
}

auto storageKind([[maybe_unused]] const U16StringSharedStorage &storage) noexcept -> StringStorageKind {
    return StringStorageKind::Shared;
}

auto storageKind([[maybe_unused]] const U32StringSharedStorage &storage) noexcept -> StringStorageKind {
    return StringStorageKind::Shared;
}

auto storageKind(const U8StringStorage &storage) noexcept -> StringStorageKind {
    if (std::holds_alternative<U8StringLiteralStorage>(storage)) {
        return StringStorageKind::Literal;
    }
    if (std::holds_alternative<U8StringSharedStorage>(storage)) {
        return StringStorageKind::Shared;
    }
    return StringStorageKind::Empty;
}

auto storageKind(const U16StringStorage &storage) noexcept -> StringStorageKind {
    if (std::holds_alternative<U16StringLiteralStorage>(storage)) {
        return StringStorageKind::Literal;
    }
    if (std::holds_alternative<U16StringSharedStorage>(storage)) {
        return StringStorageKind::Shared;
    }
    return StringStorageKind::Empty;
}

auto storageKind(const U32StringStorage &storage) noexcept -> StringStorageKind {
    if (std::holds_alternative<U32StringLiteralStorage>(storage)) {
        return StringStorageKind::Literal;
    }
    if (std::holds_alternative<U32StringSharedStorage>(storage)) {
        return StringStorageKind::Shared;
    }
    return StringStorageKind::Empty;
}

auto toString(const StringStorageKind kind) noexcept -> String {
    switch (kind) {
    case StringStorageKind::Shared:
        return "shared"_el;
    case StringStorageKind::Literal:
        return "literal"_el;
    default:
        return "empty"_el;
    }
}

}
