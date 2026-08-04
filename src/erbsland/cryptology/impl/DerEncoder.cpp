// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "DerEncoder.hpp"

#include "../../mem/Byte.hpp"
#include "../../mem/ByteBlockEditor.hpp"
#include "../../unit/ByteIndex.hpp"
#include "../../unit/ByteLength.hpp"

#include <array>

namespace erbsland::cryptology::impl::der_encoder {

auto wrap(const uint8_t tag, const mem::ConstByteSpan content) -> mem::ByteBlock {
    auto result = mem::ByteBlockEditor{};
    result.append(mem::Byte{tag});
    appendLength(result, content.size());
    result.append(content);
    return mem::ByteBlock{result};
}

auto sequence(const mem::ConstByteSpan children) -> mem::ByteBlock {
    return wrap(0x30U, children);
}

auto positiveInteger(const mem::ConstByteSpan magnitude) -> mem::ByteBlock {
    auto first = std::size_t{};
    while (first < magnitude.size() && magnitude[first].toUInt8() == 0U) {
        ++first;
    }
    if (first == magnitude.size()) {
        const auto zero = std::array<mem::Byte, 1U>{mem::Byte{}};
        return wrap(0x02U, mem::ConstByteSpan{zero});
    }
    auto content = mem::ByteBlockEditor{};
    if ((magnitude[first].toUInt8() & 0x80U) != 0U) {
        content.append(mem::Byte{});
    }
    content.append(magnitude.subspan(first));
    return wrap(0x02U, content.span());
}

auto octetString(const mem::ConstByteSpan content) -> mem::ByteBlock {
    return wrap(0x04U, content);
}

auto bitString(const mem::ConstByteSpan content) -> mem::ByteBlock {
    auto value = mem::ByteBlockEditor{};
    value.append(mem::Byte{});
    value.append(content);
    return wrap(0x03U, value.span());
}

void appendLength(mem::ByteBlockEditor &result, const std::size_t length) {
    if (length < 128U) {
        result.append(mem::Byte{static_cast<uint8_t>(length)});
        return;
    }
    auto bytes = std::array<uint8_t, sizeof(std::size_t)>{};
    auto count = std::size_t{};
    auto remaining = length;
    while (remaining != 0U) {
        bytes[bytes.size() - 1U - count] = static_cast<uint8_t>(remaining);
        remaining >>= 8U;
        ++count;
    }
    result.append(mem::Byte{static_cast<uint8_t>(0x80U | count)});
    for (auto index = bytes.size() - count; index < bytes.size(); ++index) {
        result.append(mem::Byte{bytes[index]});
    }
}

}
