// Copyright (c) 2024-2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "BytesValue.hpp"

#include "../../../text/ByteFormat.hpp"
#include "../../../text/String.hpp"

namespace erbsland::conf::impl {

auto BytesValue::toTextRepresentation() const noexcept -> text::String {
    return text::String::fromByteBlock(_value, text::ByteFormat::compact());
}

}
