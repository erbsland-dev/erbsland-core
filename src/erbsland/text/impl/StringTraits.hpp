// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../u16/impl/U16StringTraits.hpp"
#include "../u16/U16String_fwd.hpp"
#include "../u16/U16StringEditor_fwd.hpp"
#include "../u32/impl/U32StringTraits.hpp"
#include "../u32/U32String_fwd.hpp"
#include "../u32/U32StringEditor_fwd.hpp"
#include "../u8/impl/U8StringTraits.hpp"
#include "../u8/U8String_fwd.hpp"
#include "../u8/U8StringEditor_fwd.hpp"

#include <concepts>
#include <type_traits>

namespace erbsland::text::impl {

template <typename tString>
concept AnyStringType = std::is_same_v<std::remove_cvref_t<tString>, U8String> ||
    std::is_same_v<std::remove_cvref_t<tString>, U16String> || std::is_same_v<std::remove_cvref_t<tString>, U32String>;

template <typename tString>
concept AnyStringEditorType = std::is_same_v<std::remove_cvref_t<tString>, U8StringEditor> ||
    std::is_same_v<std::remove_cvref_t<tString>, U16StringEditor> ||
    std::is_same_v<std::remove_cvref_t<tString>, U32StringEditor>;

template <typename tString>
concept AnyStringOrStringEditorType = AnyStringType<tString> || AnyStringEditorType<tString>;

/// Select the width-specific implementation traits for a concrete string or editor type.
template <AnyStringOrStringEditorType tString>
using StringTraitsFor = std::conditional_t<
    std::is_same_v<std::remove_cvref_t<tString>, U8String> ||
        std::is_same_v<std::remove_cvref_t<tString>, U8StringEditor>,
    U8StringTraits,
    std::conditional_t<
        std::is_same_v<std::remove_cvref_t<tString>, U16String> ||
            std::is_same_v<std::remove_cvref_t<tString>, U16StringEditor>,
        U16StringTraits,
        U32StringTraits>>;

/// Select the shared-storage type for a concrete string or editor type.
template <AnyStringOrStringEditorType tString>
using SharedStorageFor = typename StringTraitsFor<tString>::SharedStorage;

/// Select the native data-index type for a concrete string or editor type.
template <AnyStringOrStringEditorType tString>
using StringIndexTypeFor = typename StringTraitsFor<tString>::DataIndex;

}
