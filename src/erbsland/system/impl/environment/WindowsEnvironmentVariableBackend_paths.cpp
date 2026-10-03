// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "WindowsEnvironmentVariableBackend.hpp"

#include "../../../path/PathWindowsFormat.hpp"
#include "../../../text/CharSet.hpp"
#include "../../../text/impl/PlatformU16StringAccess.hpp"
#include "../../../text/Literals.hpp"
#include "../../../text/StringConverter.hpp"
#include "../../../text/StringEditor.hpp"
#include "../../../text/StringJoins.hpp"
#include "../../../unit/ByteRange.hpp"

#include <cwchar>

namespace erbsland::system::impl {

using namespace path;
using namespace text;
using namespace unit;
using namespace text::literals;

auto WindowsEnvironmentVariableBackend::pathSeparator() const noexcept -> Char {
    return Char{U';'};
}

auto WindowsEnvironmentVariableBackend::parsePath(const String &value) const -> Path {
    return Path::fromWindows(value);
}

auto WindowsEnvironmentVariableBackend::pathText(const Path &path) const -> String {
    return path.toWindows(PathWindowsFormat::Native);
}

auto WindowsEnvironmentVariableBackend::isLiteralPath(const String &value) const -> bool {
    return !value.containsOneOf(CharSet{U'%', U'"'});
}

auto WindowsEnvironmentVariableBackend::executableName(const Path &path) const -> String {
    if (path.suffix().isEmpty()) {
        return text::join(path.name(), ".exe"_el);
    }
    return path.name();
}

auto WindowsEnvironmentVariableBackend::compare(const String &left, const String &right) const -> std::strong_ordering {
    return compare(StringConverter{left}.toU16String(), StringConverter{right}.toU16String());
}

auto WindowsEnvironmentVariableBackend::compare(const U16String &left, const U16String &right) const
    -> std::strong_ordering {
    const auto first = text::impl::PlatformU16StringAccess{left};
    const auto second = text::impl::PlatformU16StringAccess{right};
    return ::_wcsicmp(first.nullTerminatedWideCharPtr(), second.nullTerminatedWideCharPtr()) <=> 0;
}

auto WindowsEnvironmentVariableBackend::expandPath(const String &value, const Lookup &lookup) const
    -> std::optional<String> {
    static const auto invalidExpansion = CharSet{U'%', U';', U'\0'};
    static const auto invalidPath = CharSet{U'"', U';', U'\0'};
    auto output = StringEditor{};
    auto remaining = value;
    while (!remaining.isEmpty()) {
        const auto opening = remaining.splitAtFirst("%"_el);
        if (!opening) {
            output.append(remaining);
            break;
        }
        output.append(opening->first);
        const auto closing = opening->second.splitAtFirst("%"_el);
        if (!closing || closing->first.isEmpty()) {
            return {};
        }
        const auto &name = closing->first;
        if (name.containsOneOf(CharSet{U'=', U'\0'})) {
            return {};
        }
        const auto replacement = lookup(name);
        if (!replacement || !replacement->isEncodingValid() || replacement->containsOneOf(invalidExpansion)) {
            return {};
        }
        output.append(*replacement);
        remaining = closing->second;
    }
    auto result = String{output};
    if (result.startsWith("\""_el) && result.endsWith("\""_el) && result.length() >= ByteCount{2}) {
        result = result.slice(ByteRange{ByteIndex{1}, ByteIndex::last(result.length())});
    }
    if (result.containsOneOf(invalidPath)) {
        return {};
    }
    return result;
}

}
