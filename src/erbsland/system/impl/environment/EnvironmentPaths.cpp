// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "EnvironmentPaths.hpp"

#include "../../../err/ParameterFailure.hpp"
#include "../../../text/Literals.hpp"
#include "../../../text/StringEditor.hpp"
#include "../../../text/StringSplitter.hpp"

namespace erbsland::system::impl {

using namespace path;
using namespace text;
using namespace text::literals;

EnvironmentPaths::EnvironmentPaths(const EnvironmentVariableBackend &backend, Lookup lookup) :
    _backend{backend}, _lookup{std::move(lookup)} {
}

void EnvironmentPaths::appendEntry(const String &spelling) {
    if (!spelling.isEncodingValid() || spelling.containsOneOf(CharSet{U'\0'})) {
        return;
    }
    const auto value = _backend.expandPath(spelling, _lookup);
    if (!value) {
        return;
    }
    const auto key = _backend.pathKey(*value);
    if (!key.isEmpty() && find(key).isNoIndex()) {
        _entries.append(Entry{spelling, key});
    }
}

void EnvironmentPaths::read(const String &value) {
    auto splitter = StringSplitter{value, _backend.pathSeparator()};
    while (!splitter.isAtEnd()) {
        appendEntry(splitter.next());
    }
}

auto EnvironmentPaths::paths() const -> PathList {
    auto result = PathList{};
    result.reserve(_entries.count());
    for (const auto &entry : _entries) {
        result.append(entry.key);
    }
    return result;
}

auto EnvironmentPaths::text() const -> String {
    auto spellings = StringList{};
    spellings.reserve(_entries.count());
    for (const auto &entry : _entries) {
        spellings.append(entry.spelling);
    }
    return spellings.join(String::fromCharacter(_backend.pathSeparator()));
}

auto EnvironmentPaths::find(const Path &key) const -> unit::ItemIndex {
    return _entries.findFirstIf([&](const Entry &entry) -> bool { return _backend.samePath(entry.key, key); });
}

void EnvironmentPaths::validate(const Path &path, const EnvironmentVariableBackend &backend) {
    const auto spelling = backend.pathText(path);
    if (path.isEmpty() || !path.isValid() || !spelling.isEncodingValid() ||
        spelling.containsOneOf(CharSet{backend.pathSeparator()}) || !backend.isLiteralPath(spelling) ||
        backend.pathKey(spelling).isEmpty()) {
        throw err::ParameterFailure{
            "PATH arguments must be valid paths without separators or ambiguous syntax."_el, "path"_el};
    }
}

void EnvironmentPaths::insert(unit::ItemIndex index, const Path &path) {
    validate(path, _backend);
    if (index.isNoIndex() || index > unit::ItemIndex::end(_entries.count())) {
        throw err::ParameterFailure{"The PATH insertion index is outside the list."_el, "index"_el};
    }
    const auto spelling = _backend.pathText(path);
    const auto key = _backend.pathKey(spelling);
    const auto previous = find(key);
    if (!previous.isNoIndex()) {
        _entries.remove(previous);
        if (previous < index) {
            --index;
        }
    }
    _entries.insert(index, Entry{spelling, key});
}

void EnvironmentPaths::append(const Path &path) {
    validate(path, _backend);
    appendEntry(_backend.pathText(path));
}

void EnvironmentPaths::remove(const Path &path, const bool descendants) {
    validate(path, _backend);
    const auto key = _backend.pathKey(_backend.pathText(path));
    _entries.removeIf([&](const Entry &entry) -> bool {
        if (descendants) {
            return _backend.inBase(entry.key, key);
        }
        return _backend.samePath(entry.key, key);
    });
}

auto EnvironmentPaths::executableCandidates(const String &name) const -> PathList {
    const auto requested = _backend.pathKey(name);
    if (name.isEmpty() || requested.isEmpty() || requested.name().isEmpty() || name == "."_el || name == ".."_el ||
        (!requested.isAbsolute() && requested.name() != name)) {
        throw err::ParameterFailure{"An executable must be a name or an absolute native path."_el, "name"_el};
    }
    const auto filename = _backend.executableName(requested);
    if (requested.isAbsolute()) {
        return PathList{requested.parent() / filename};
    }
    auto result = PathList{};
    result.reserve(_entries.count());
    for (const auto &entry : _entries) {
        result.append(entry.key / filename);
    }
    return result;
}

}
