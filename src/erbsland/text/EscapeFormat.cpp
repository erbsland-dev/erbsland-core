// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "EscapeFormat.hpp"

#include "Literals.hpp"
#include "String.hpp"

#include "impl/ThrowHelper.hpp"

namespace erbsland::text {

using namespace text::literals;

auto EscapeFormat::toString() const -> StringView {
    switch (_value) {
    case Html:
        return "html"_el;
    case Json:
        return "json"_el;
    case Cpp:
        return "cpp"_el;
    case Xml:
        return "xml"_el;
    case PCRE:
        return "pcre"_el;
    case Display:
        return "display"_el;
    case None:
    default:
        return "none"_el;
    }
}

auto EscapeFormat::fromString(const StringView &text) noexcept -> std::optional<EscapeFormat> {
    if (text == "none"_el) {
        return EscapeFormat{None};
    }
    if (text == "html"_el) {
        return EscapeFormat{Html};
    }
    if (text == "json"_el) {
        return EscapeFormat{Json};
    }
    if (text == "cpp"_el) {
        return EscapeFormat{Cpp};
    }
    if (text == "xml"_el) {
        return EscapeFormat{Xml};
    }
    if (text == "pcre"_el) {
        return EscapeFormat{PCRE};
    }
    if (text == "display"_el) {
        return EscapeFormat{Display};
    }
    return {};
}

auto EscapeFormat::fromStringOrThrow(const StringView &text) -> EscapeFormat {
    if (const auto result = fromString(text); result.has_value()) {
        return result.value();
    }
    text::impl::throwParseError("Unsupported escape format");
}

}
