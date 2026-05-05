// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

#include <string>
#include <string_view>

/// Demonstrates how to convert between library strings and standard string views using `StringConverter`.
void crossConvertStrings() {
    // Convert between all library string types.
    auto u8String = el::U8String{u8"Bonjour, forêt 🌲"};
    auto u16String = el::StringConverter{u8String}.toU16String();
    auto u32String = el::StringConverter{u16String}.toU32String();
    auto backToU8String = el::StringConverter{u32String}.toU8String();
    el::io::printLine("After conversion: ", backToU8String);

    // Convert library strings to standard library strings.
    auto stdString = el::StringConverter{u32String}.toStdString();
    el::io::printLine("Converted stdString: ", stdString);

    // Convert standard string views.
    constexpr auto stdStringView = std::string_view{"Hello, river"};
    constexpr auto stdU8StringView = std::u8string_view{u8"Bonjour, forêt 🌲"};
    constexpr auto stdU16StringView = std::u16string_view{u"Hola, río"};
    constexpr auto stdU32StringView = std::u32string_view{U"Ciao, sole ☀"};
    constexpr auto stdWStringView = std::wstring_view{L"Hej, skog"};

    u8String = el::StringConverter{stdStringView}.toU8String();
    el::io::printLine("Converted 'stdStringView': ", u8String);

    u8String = el::StringConverter{stdU8StringView}.toU8String();
    el::io::printLine("Converted 'stdU8StringView': ", u8String);

    u8String = el::StringConverter{stdU16StringView}.toU8String();
    el::io::printLine("Converted 'stdU16StringView': ", u8String);

    u8String = el::StringConverter{stdU32StringView}.toU8String();
    el::io::printLine("Converted 'stdU32StringView': ", u8String);

    u8String = el::StringConverter{stdWStringView}.toU8String();
    el::io::printLine("Converted 'stdWStringView': ", u8String);

    // Convert a standard string.
    stdString = std::string{"Hello, wind"};
    u8String = el::StringConverter{stdString}.toU8String();
    el::io::printLine("Converted 'stdString': ", u8String);
}
