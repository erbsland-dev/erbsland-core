// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/text/impl/UnsafeU8StringBuffer.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/String.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/text/StringView.hpp>
#include <erbsland/text/StringViewList.hpp>

#include <cstdint>
#include <initializer_list>
#include <string>

using el::text::String;
using el::text::StringView;
using namespace el::text::literals;

namespace re_test::string_helper {

inline auto bytesToString(const std::initializer_list<std::uint8_t> bytes) -> String {
    auto buffer = el::text::impl::UnsafeU8StringBuffer{el::unit::ByteLength::fromSizeT(bytes.size())};
    auto index = std::size_t{};
    for (const auto byte : bytes) {
        buffer.data()[index] = static_cast<char>(byte);
        ++index;
    }
    return buffer.take();
}

inline auto bytesToStdString(const std::initializer_list<std::uint8_t> bytes) -> std::string {
    std::string result;
    result.reserve(bytes.size());
    for (const auto byte : bytes) {
        result.push_back(static_cast<char>(byte));
    }
    return result;
}

inline auto bytesToU8String(const std::initializer_list<std::uint8_t> bytes) -> std::u8string {
    std::u8string result;
    result.reserve(bytes.size());
    for (const auto byte : bytes) {
        result.push_back(static_cast<char8_t>(byte));
    }
    return result;
}

[[nodiscard]] inline auto toStdString(const StringView &value) -> std::string {
    return el::text::StringConverter{value}.toStdString();
}

[[nodiscard]] inline auto toStringViewList(const std::initializer_list<std::string_view> lines)
    -> el::text::StringViewList {
    el::text::StringViewList result;
    for (const auto line : lines) {
        result.append(String{line});
    }
    return result;
}

}
