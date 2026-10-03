// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PosixEnvironmentVariableBackend.hpp"

#include "../../../text/impl/UnsafeU16StringAccess.hpp"
#include "../../../text/impl/UnsafeU8StringAccess.hpp"

#include <algorithm>

namespace erbsland::system::impl {

using namespace path;
using namespace text;
using text::impl::UnsafeU16StringAccess;
using text::impl::UnsafeU8StringAccess;

auto PosixEnvironmentVariableBackend::pathSeparator() const noexcept -> Char {
    return Char{U':'};
}

auto PosixEnvironmentVariableBackend::parsePath(const String &value) const -> Path {
    return Path::fromPosix(value);
}

auto PosixEnvironmentVariableBackend::pathText(const Path &path) const -> String {
    return path.toPosix();
}

auto PosixEnvironmentVariableBackend::expandPath(const String &value, const Lookup &) const -> std::optional<String> {
    return value;
}

auto PosixEnvironmentVariableBackend::isLiteralPath(const String &) const -> bool {
    return true;
}

auto PosixEnvironmentVariableBackend::executableName(const Path &path) const -> String {
    return path.name();
}

auto PosixEnvironmentVariableBackend::compare(const String &left, const String &right) const -> std::strong_ordering {
    const auto first = UnsafeU8StringAccess{left}.dataSpan();
    const auto second = UnsafeU8StringAccess{right}.dataSpan();
    return std::lexicographical_compare_three_way(first.begin(), first.end(), second.begin(), second.end());
}

auto PosixEnvironmentVariableBackend::compare(const U16String &left, const U16String &right) const
    -> std::strong_ordering {
    const auto first = UnsafeU16StringAccess{left}.dataSpan();
    const auto second = UnsafeU16StringAccess{right}.dataSpan();
    return std::lexicographical_compare_three_way(first.begin(), first.end(), second.begin(), second.end());
}

}
