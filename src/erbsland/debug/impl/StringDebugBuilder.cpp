// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "StringDebugBuilder.hpp"

#include "../../text/IntegerFormatFlag.hpp"

namespace erbsland::debug::impl {

using namespace text::literals;

auto storageKindText(const StringStorageKind kind) noexcept -> text::StringView {
    switch (kind) {
    case StringStorageKind::Shared:
        return "shared"_el;
    case StringStorageKind::Literal:
        return "literal"_el;
    default:
        return "empty"_el;
    }
}

auto storageIdentifierText(const mem::StorageIdentifier &storageId) -> text::String {
    if (storageId.isEmpty()) {
        return text::String{"empty"_el};
    }
    const auto values = storageId.toRawValues();
    auto builder = text::StringBuilder{};
    const auto format = text::IntegerFormat::hexadecimal().setFlags(
        text::IntegerFormatFlag::BasePrefix | text::IntegerFormatFlag::ZeroFill);
    builder.appendInteger(values[0], format);
    builder.append(U':');
    builder.appendInteger(values[1], format);
    return builder.toU8String();
}

}
