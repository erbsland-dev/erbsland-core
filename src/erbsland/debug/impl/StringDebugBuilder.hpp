// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "StringDebugAccess.hpp"

#include "../DebugViewDetails.hpp"

#include "../../text/AnyStringView.hpp"
#include "../../text/SafeStringFlag.hpp"
#include "../../text/StringBuilder.hpp"
#include "../../text/StringFormat.hpp"
#include "../../text/StringTree.hpp"

#include <utility>

namespace erbsland::debug::impl {

/// The width for content previews in string debug titles.
constexpr auto cContentsPreviewWidth = unit::CpLength{60U};

/// Convert a string storage kind to debug text.
[[nodiscard]] auto storageKindText(StringStorageKind kind) noexcept -> text::StringView;

/// Convert a storage identifier to debug text.
[[nodiscard]] auto storageIdentifierText(const mem::StorageIdentifier &storageId) -> text::String;

/// Append a byte/code-unit/code-point range to the debug tree.
/// @tested{StringDebugBuilderTest}
template <typename T>
void appendRange(text::StringTree &tree, const T &range) {
    using namespace text::literals;

    static const auto rangeFormat = text::StringFormat{"index: {} - {} (length: {})"};
    tree.append("selectedRange"_el, rangeFormat.build(range.index(), range.endIndex(), range.length()));
}

/// Append memory details to the debug tree.
/// @tested{StringDebugBuilderTest}
template <typename T>
void appendStorage(text::StringTree &tree, const T &value, const DebugViewDetails details) {
    using namespace text::literals;

    const auto dataView = StringDebugAccess::dataView(value);
    if (details.isSet(DebugViewDetail::UnderlyingType) || details.isSet(DebugViewDetail::CoreDetails)) {
        tree.append("storageKind"_el, storageKindText(StringDebugAccess::storageKind(value)));
    }
    if (details.isSet(DebugViewDetail::StorageId)) {
        tree.append("storageId"_el, storageIdentifierText(value.storageId()));
    }
    tree.append("backingStorageId"_el, storageIdentifierText(StringDebugAccess::backingStorageId(value)));
    if (details.isSet(DebugViewDetail::States) || details.isSet(DebugViewDetail::CoreDetails)) {
        tree.append("isShared"_el, StringDebugAccess::isShared(value));
    }
    if (details.isSet(DebugViewDetail::Size) || details.isSet(DebugViewDetail::CoreDetails)) {
        tree.append("backingLength"_el, dataView.data().size());
    }
    if (details.isSet(DebugViewDetail::Range) || details.isSet(DebugViewDetail::CoreDetails)) {
        appendRange(tree, dataView.range());
    }
}

/// Create the title text for a string debug tree.
/// @tested{StringDebugBuilderTest}
template <typename T>
[[nodiscard]] auto labelWithContents(text::StringView typeName, const T &value, const DebugViewDetails details)
    -> text::String {
    auto builder = text::StringBuilder{};
    builder.append(typeName);
    if (details.isSet(DebugViewDetail::ContentInTitle)) {
        builder.append(U'(');
        builder.append(value.toSafeString(cContentsPreviewWidth, text::SafeStringFlag::Defaults));
        builder.append(U')');
    }
    return builder.toU8String();
}

/// Build a debug tree for any string or string view type.
/// @tested{StringDebugBuilderTest}
template <typename T>
[[nodiscard]] auto makeStringDebugTree(text::StringView typeName, const T &value, const DebugViewDetails details)
    -> text::StringTree {
    using namespace text::literals;

    const auto anyView = text::AnyStringView{value};
    auto result = text::StringTree{labelWithContents(typeName, value, details)};
    if (details.isSet(DebugViewDetail::States) || details.isSet(DebugViewDetail::CoreDetails)) {
        result.append("isEmpty"_el, value.isEmpty());
    }
    if (details.isSet(DebugViewDetail::DataValidity) || details.isSet(DebugViewDetail::CoreDetails)) {
        result.append("isEncodingValid"_el, anyView.isEncodingValid());
    }
    if (details.isSet(DebugViewDetail::Size) || details.isSet(DebugViewDetail::CoreDetails)) {
        result.append("length"_el, value.length());
        result.append("characterLength"_el, anyView.characterLength());
    }
    if (details.isSet(DebugViewDetail::BackingStore)) {
        appendStorage(result, value, details);
    }
    return result;
}

}
