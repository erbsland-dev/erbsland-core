// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "SyslogTransport.hpp"

#include "../err/ParseError.hpp"
#include "../text/CaseSensitivity.hpp"
#include "../text/Literals.hpp"
#include "../text/StringList.hpp"

namespace erbsland::log {

using namespace text::literals;

auto SyslogTransport::toString() const -> text::String {
    switch (_value) {
    case Udp:
        return "udp"_el;
    case Tcp:
        return "tcp"_el;
    case Tls:
        return "tls"_el;
    }
    return {};
}

auto SyslogTransport::allStrings() -> text::StringList {
    return text::StringList{"udp"_el, "tcp"_el, "tls"_el};
}

auto SyslogTransport::fromString(const text::String &text) noexcept -> std::optional<SyslogTransport> {
    const auto compare = ::erbsland::text::cCaseInsensitive.asciiComparisonFn();
    if (text.compare("udp"_el, compare) == std::strong_ordering::equal) {
        return Udp;
    }
    if (text.compare("tcp"_el, compare) == std::strong_ordering::equal) {
        return Tcp;
    }
    if (text.compare("tls"_el, compare) == std::strong_ordering::equal) {
        return Tls;
    }
    return std::nullopt;
}

auto SyslogTransport::fromStringOrThrow(const text::String &text) -> SyslogTransport {
    if (const auto result = fromString(text); result.has_value()) {
        return *result;
    }
    throw err::ParseError{"Unsupported syslog transport."_el};
}

}
