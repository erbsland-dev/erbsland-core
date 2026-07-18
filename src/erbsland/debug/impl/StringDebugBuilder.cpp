// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "StringDebugBuilder.hpp"

#include "../../text/IntegerFormatFlag.hpp"

namespace erbsland::debug::impl {

using namespace text::literals;
using namespace text;

auto storageKindText(const StringStorageKind kind) noexcept -> String {
    switch (kind) {
    case StringStorageKind::Shared:
        return "shared"_el;
    case StringStorageKind::Literal:
        return "literal"_el;
    default:
        return "empty"_el;
    }
}

auto storageIdentifierText(const mem::StorageIdentifier &storageId) -> String {
    if (storageId.isEmpty()) {
        return "empty"_el;
    }
    const auto values = storageId.toRawValues();
    const auto format =
        IntegerFormat::hexadecimal().setFlags(IntegerFormatFlag::BasePrefix | IntegerFormatFlag::ZeroFill);
    return String::fromJoined({String::fromInteger(values[0], format), ":"_el, String::fromInteger(values[1], format)});
}

}
