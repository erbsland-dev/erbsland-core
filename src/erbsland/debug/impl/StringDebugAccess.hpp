// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "StringDebugAccess_fwd.hpp"

#include "../../mem/StorageIdentifier.hpp"
#include "../../text/u16/impl/U16StringLiteralStorage.hpp"
#include "../../text/u16/impl/U16StringSharedStorage.hpp"
#include "../../text/u16/U16String.hpp"
#include "../../text/u16/U16StringView.hpp"
#include "../../text/u32/impl/U32StringLiteralStorage.hpp"
#include "../../text/u32/impl/U32StringSharedStorage.hpp"
#include "../../text/u32/U32String.hpp"
#include "../../text/u32/U32StringView.hpp"
#include "../../text/u8/impl/U8StringLiteralStorage.hpp"
#include "../../text/u8/impl/U8StringSharedStorage.hpp"
#include "../../text/u8/U8String.hpp"
#include "../../text/u8/U8StringView.hpp"

#include <span>
#include <variant>

namespace erbsland::debug::impl {

enum class StringStorageKind {
    Empty,
    Shared,
    Literal,
};

class StringDebugAccess final {
public:
    [[nodiscard]] static auto dataView(const text::U8String &value) noexcept -> text::impl::U8StringDataView {
        return value.dataView();
    }
    [[nodiscard]] static auto dataView(const text::U8StringView &value) noexcept -> text::impl::U8StringDataView {
        return value.dataView();
    }
    [[nodiscard]] static auto dataView(const text::U16String &value) noexcept -> text::impl::U16StringDataView {
        return value.dataView();
    }
    [[nodiscard]] static auto dataView(const text::U16StringView &value) noexcept -> text::impl::U16StringDataView {
        return value.dataView();
    }
    [[nodiscard]] static auto dataView(const text::U32String &value) noexcept -> text::impl::U32StringDataView {
        return value.dataView();
    }
    [[nodiscard]] static auto dataView(const text::U32StringView &value) noexcept -> text::impl::U32StringDataView {
        return value.dataView();
    }

    [[nodiscard]] static auto storageKind(const text::U8String &) noexcept -> StringStorageKind {
        return StringStorageKind::Shared;
    }
    [[nodiscard]] static auto storageKind(const text::U16String &) noexcept -> StringStorageKind {
        return StringStorageKind::Shared;
    }
    [[nodiscard]] static auto storageKind(const text::U32String &) noexcept -> StringStorageKind {
        return StringStorageKind::Shared;
    }
    [[nodiscard]] static auto storageKind(const text::U8StringView &value) noexcept -> StringStorageKind {
        return viewStorageKind<text::impl::U8StringSharedStorage, text::impl::U8StringLiteralStorage>(value._storage);
    }
    [[nodiscard]] static auto storageKind(const text::U16StringView &value) noexcept -> StringStorageKind {
        return viewStorageKind<text::impl::U16StringSharedStorage, text::impl::U16StringLiteralStorage>(value._storage);
    }
    [[nodiscard]] static auto storageKind(const text::U32StringView &value) noexcept -> StringStorageKind {
        return viewStorageKind<text::impl::U32StringSharedStorage, text::impl::U32StringLiteralStorage>(value._storage);
    }

    [[nodiscard]] static auto isShared(const text::U8String &value) noexcept -> bool {
        return value._storage.sharedData().isShared();
    }
    [[nodiscard]] static auto isShared(const text::U16String &value) noexcept -> bool {
        return value._storage.sharedData().isShared();
    }
    [[nodiscard]] static auto isShared(const text::U32String &value) noexcept -> bool {
        return value._storage.sharedData().isShared();
    }
    [[nodiscard]] static auto isShared(const text::U8StringView &value) noexcept -> bool {
        return viewShared<text::impl::U8StringSharedStorage>(value._storage);
    }
    [[nodiscard]] static auto isShared(const text::U16StringView &value) noexcept -> bool {
        return viewShared<text::impl::U16StringSharedStorage>(value._storage);
    }
    [[nodiscard]] static auto isShared(const text::U32StringView &value) noexcept -> bool {
        return viewShared<text::impl::U32StringSharedStorage>(value._storage);
    }

    template <typename T>
    [[nodiscard]] static auto backingStorageId(const T &value) noexcept -> mem::StorageIdentifier {
        const auto data = dataView(value).data();
        if (data.data() == nullptr) {
            return {};
        }
        return mem::StorageIdentifier::fromMemoryRange(data.data(), data.data() + data.size());
    }

private:
    template <typename SharedStorage, typename LiteralStorage, typename Storage>
    [[nodiscard]] static auto viewStorageKind(const Storage &storage) noexcept -> StringStorageKind {
        if (std::holds_alternative<SharedStorage>(storage)) {
            return StringStorageKind::Shared;
        }
        if (std::holds_alternative<LiteralStorage>(storage)) {
            return StringStorageKind::Literal;
        }
        return StringStorageKind::Empty;
    }

    template <typename SharedStorage, typename Storage>
    [[nodiscard]] static auto viewShared(const Storage &storage) noexcept -> bool {
        if (const auto *shared = std::get_if<SharedStorage>(&storage)) {
            return shared->sharedData().isShared();
        }
        return false;
    }
};

}
