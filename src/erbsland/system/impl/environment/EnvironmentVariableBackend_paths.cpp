// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "EnvironmentVariableBackend.hpp"

#include "../../../path/impl/PathLexical.hpp"
#include "../../../text/Literals.hpp"

namespace erbsland::system::impl {

using namespace path;
using namespace text;
using namespace text::literals;

auto EnvironmentVariableBackend::native() -> const EnvironmentVariableBackend & {
    static const auto backend = createEnvironmentVariableBackend();
    return *backend;
}

auto EnvironmentVariableBackend::pathKey(const String &value) const -> Path {
    if (!value.isEncodingValid()) {
        return {};
    }
    if (value.isEmpty()) {
        return Path::currentElement();
    }
    return path::impl::lexicalPath(parsePath(value));
}

auto EnvironmentVariableBackend::samePath(const Path &left, const Path &right) const -> bool {
    return left.elementCount() == right.elementCount() && inBase(left, right) && inBase(right, left);
}

auto EnvironmentVariableBackend::inBase(const Path &path, const Path &base) const -> bool {
    if (path.isEmpty() || base.isEmpty() || path.isAbsolute() != base.isAbsolute() ||
        compare(path.root(), base.root()) != 0) {
        return false;
    }
    auto elements = path.elements();
    auto baseElements = base.elements();
    // A normalized relative '.' is the base of relative paths that do not escape through '..'.
    if (base == Path::currentElement()) {
        return elements.first() != ".."_el;
    }
    if (elements.count() < baseElements.count()) {
        return false;
    }
    for (auto index = unit::ItemIndex::zero(); index.isWithin(baseElements.count()); ++index) {
        if (compare(elements.getRef(index), baseElements.getRef(index)) != 0) {
            return false;
        }
    }
    return true;
}

auto EnvironmentVariableBackend::pathSeparator() const noexcept -> Char {
    return native().pathSeparator();
}

auto EnvironmentVariableBackend::compare(const String &left, const String &right) const -> std::strong_ordering {
    return native().compare(left, right);
}

auto EnvironmentVariableBackend::compare(const U16String &left, const U16String &right) const -> std::strong_ordering {
    return native().compare(left, right);
}

auto EnvironmentVariableBackend::parsePath(const String &value) const -> Path {
    return native().parsePath(value);
}

auto EnvironmentVariableBackend::pathText(const Path &path) const -> String {
    return native().pathText(path);
}

auto EnvironmentVariableBackend::expandPath(const String &value, const Lookup &lookup) const -> std::optional<String> {
    return native().expandPath(value, lookup);
}

auto EnvironmentVariableBackend::isLiteralPath(const String &value) const -> bool {
    return native().isLiteralPath(value);
}

auto EnvironmentVariableBackend::executableName(const Path &path) const -> String {
    return native().executableName(path);
}

}
