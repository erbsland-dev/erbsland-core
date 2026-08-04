// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "CryptographicDataBlock.hpp"

#include "../../text/String.hpp"

#include <utility>

namespace erbsland::cryptology {

CryptographicDataBlock::CryptographicDataBlock(mem::ByteBlock data) noexcept : _data{std::move(data)} {
    _data.markAsSensitive();
}

CryptographicDataBlock::CryptographicDataBlock(const mem::ConstByteSpan data) :
    CryptographicDataBlock{mem::ByteBlock::fromSpan(data)} {
}

auto CryptographicDataBlock::dataToString() const -> text::String {
    return text::String::fromByteBlock(_data);
}

}
