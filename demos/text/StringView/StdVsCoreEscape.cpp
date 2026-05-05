// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>

/// Erbsland Core uses a rich API approach to keep boilerplate to a minimum.
/// The common case stays short, readable, and maintainable.
/// More dangerous and error-prone code is intentionally more explicit.
/// This demo compares Erbsland Core code with equivalent standard-library code.
void stdVsCoreEscape() {
    el::io::printLine("=== Escaping text for C++ ==="_el);

    // Erbsland Core: Built-in escaping with a named target format.
    constexpr auto elText = "\"\"\"x\ny\"\"\""_el;
    const auto coreEscaped = el::StringView{elText}.toEscaped(el::EscapeFormat::Cpp);
    el::io::printLine("Core: "_el, coreEscaped);

    // Standard library: No direct C++ string escaping API; write the rules manually.
    constexpr auto stdText = std::string_view{"\"\"\"x\ny\"\"\""};
    std::string stdEscaped;
    for (const auto character : stdText) {
        switch (character) {
        case '\n':
            stdEscaped += "\\n";
            break;
        case '\r':
            stdEscaped += "\\r";
            break;
        case '\t':
            stdEscaped += "\\t";
            break;
        case '"':
            stdEscaped += "\\\"";
            break;
        case '\\':
            stdEscaped += "\\\\";
            break;
        default:
            stdEscaped += character;
            break;
        }
    }
    el::io::printLine("std:  "_el, stdEscaped);
}
