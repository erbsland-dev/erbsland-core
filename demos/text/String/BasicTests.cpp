// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// Create a string that deliberately contains invalid UTF-8 bytes.
///
/// This is only for demonstrating error handling. Do not construct strings this
/// way in application code.
auto createTextWithInvalidUtf8() -> el::String {
    constexpr auto bytes = std::array<const char, 15>{
        'S', 'u', 'n', 'n', 'y', ' ', static_cast<char>(0x82U), ' ', 'W', 'e', 'a', 't', 'h', 'e', 'r'};

    return el::String{std::string_view{bytes.data(), bytes.size()}};
}

/// This demo shows basic state checks for `String`.
///
/// It demonstrates:
/// - checking whether a string is empty,
/// - checking whether its byte data is valid UTF-8,
/// - safely printing strings even when invalid UTF-8 is present.
///
/// Invalid UTF-8 data is handled safely. Encoding errors are represented with
/// the Unicode replacement character U+FFFD.
void basicTests() {
    const auto station = el::String{"🌦️ Station Süd: Nebel über dem Fjord"_el};
    const auto empty = el::String{};
    const auto invalidUtf8 = createTextWithInvalidUtf8();

    el::io::printLine("Input strings:"_el);
    el::io::printLine("  station     : \""_el, station, "\""_el);
    el::io::printLine("  empty       : \""_el, empty, "\""_el);
    el::io::printLine("  invalidUtf8 : \""_el, invalidUtf8, "\""_el);

    // `isEmpty()` checks whether the string contains no bytes.
    el::io::printLine("\nEmpty checks:"_el);
    el::io::printLine("  station.isEmpty()     : "_el, station.isEmpty());
    el::io::printLine("  empty.isEmpty()       : "_el, empty.isEmpty());
    el::io::printLine("  invalidUtf8.isEmpty() : "_el, invalidUtf8.isEmpty());

    // `isValidUtf8()` checks whether all bytes form valid UTF-8 sequences.
    el::io::printLine("\nUTF-8 validity checks:"_el);
    el::io::printLine("  station.isValidUtf8()     : "_el, station.isValidUtf8());
    el::io::printLine("  empty.isValidUtf8()       : "_el, empty.isValidUtf8());
    el::io::printLine("  invalidUtf8.isValidUtf8() : "_el, invalidUtf8.isValidUtf8());
}

}
