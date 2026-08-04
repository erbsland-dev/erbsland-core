// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "StringDebugAccess_fwd.hpp"

#include "../../mem/StorageIdentifier.hpp"
#include "../../text/impl/StringStorageKind.hpp"
#include "../../text/impl/StringTraits.hpp"
#include "../../text/u16/impl/U16StringData.hpp"
#include "../../text/u16/impl/U16StringLiteralStorage.hpp"
#include "../../text/u16/impl/U16StringSharedStorage.hpp"
#include "../../text/u16/U16String.hpp"
#include "../../text/u16/U16StringEditor.hpp"
#include "../../text/u32/impl/U32StringData.hpp"
#include "../../text/u32/impl/U32StringLiteralStorage.hpp"
#include "../../text/u32/impl/U32StringSharedStorage.hpp"
#include "../../text/u32/U32String.hpp"
#include "../../text/u32/U32StringEditor.hpp"
#include "../../text/u8/impl/U8StringData.hpp"
#include "../../text/u8/impl/U8StringLiteralStorage.hpp"
#include "../../text/u8/impl/U8StringSharedStorage.hpp"
#include "../../text/u8/U8String.hpp"

#include <span>
#include <variant>

namespace erbsland::debug::impl {

/// Provide debug-only access to a string without copying its storage.
/// @tparam tString The editable or read-only string type.
/// @tested{StringDebugBuilderTest}
template <text::impl::AnyStringOrStringEditorType tString>
class StringDebugAccess final {
    using StringStorageKind = text::impl::StringStorageKind;

public:
    /// Create debug access for a string value.
    /// @param value The string value to inspect.
    explicit StringDebugAccess(const tString &value) noexcept : _value{value} {}

    /// Get the string's data view.
    [[nodiscard]] auto dataView() const noexcept { return _value.dataView(); }

    /// Get the kind of storage backing the string.
    [[nodiscard]] auto storageKind() const noexcept -> StringStorageKind {
        if constexpr (text::impl::AnyStringEditorType<tString>) {
            return StringStorageKind::Shared;
        } else if constexpr (text::impl::AnyStringType<tString>) {
            return text::impl::storageKind(_value._storage);
        } else {
            std::terminate();
        }
    }

    /// Test whether the string's backing storage is shared.
    [[nodiscard]] auto isShared() const noexcept -> bool { return _value.isStorageShared(); }

    /// Get an identifier for the complete backing storage.
    [[nodiscard]] auto backingStorageId() const noexcept -> mem::StorageIdentifier {
        const auto data = dataView().data();
        if (data.data() == nullptr) {
            return {};
        }
        return mem::StorageIdentifier::fromMemoryRange(data.data(), data.data() + data.size());
    }

private:
    const tString &_value;
};

}
