// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

namespace erbsland::system::impl {

template <typename T>
auto EnvironmentChangesProcessor::encode(const text::String &value) -> T {
    if constexpr (std::is_same_v<T, text::String>) {
        return value;
    } else {
        return text::StringConverter{value}.toU16String();
    }
}

template <typename T>
auto EnvironmentChangesProcessor::decode(const T &value) -> std::optional<text::String> {
    if constexpr (std::is_same_v<T, text::String>) {
        if (value.isEncodingValid()) {
            return value;
        }
    } else {
        if (value.isEncodingValid()) {
            return text::StringConverter{value}.toString();
        }
    }
    return {};
}

template <typename T>
auto EnvironmentChangesProcessor::apply(const EnvironmentChanges &changes, const Environment<T> &base)
    -> Environment<T> {
    auto result = Environment<T>{base.key_comp()};
    if (changes._inheritedNames.has_value()) {
        for (const auto &name : *changes._inheritedNames) {
            const auto entry = base.find(encode<T>(name));
            if (entry != base.end()) {
                result.insert(*entry);
            }
        }
    } else if (changes._inherit == EnvironmentInherit::All) {
        result = base;
    }
    for (const auto &[name, value] : changes._variables) {
        const auto nativeName = encode<T>(name);
        if (value.has_value()) {
            result.insert_or_assign(nativeName, encode<T>(*value));
        } else {
            result.erase(nativeName);
        }
    }
    if (changes.hasPathChanges()) {
        applyPath(changes, result);
    }
    return result;
}

template <typename T>
void EnvironmentChangesProcessor::applyPath(const EnvironmentChanges &changes, Environment<T> &environment) {
    using namespace text::literals;
    const auto comparator = environment.key_comp();
    const auto &backend = comparator.backend.get();
    const auto name = encode<T>(EnvironmentVariableBackend::cPathVariable);
    const auto existing = environment.find(name);
    if (existing == environment.end() && changes._frontPaths.isEmpty() && changes._backPaths.isEmpty()) {
        return;
    }
    auto paths = EnvironmentPaths{backend, [&](const text::String &variable) -> std::optional<text::String> {
                                      const auto found = environment.find(encode<T>(variable));
                                      if (found == environment.end()) {
                                          return {};
                                      }
                                      return decode(found->second);
                                  }};
    if (existing != environment.end()) {
        // Split native text before conversion so a malformed entry cannot discard valid neighbors.
        auto splitter = text::impl::StringSplitter<T>{existing->second, backend.pathSeparator()};
        while (!splitter.isAtEnd()) {
            if (const auto entry = decode(splitter.next())) {
                paths.appendEntry(*entry);
            }
        }
    }
    for (const auto &path : changes._removedPaths) {
        paths.remove(path);
    }
    for (const auto &base : changes._removedBases) {
        paths.remove(base, true);
    }
    // Prepend in reverse order; the descriptor already stores most recent additions first.
    for (auto index = unit::ItemIndex::end(changes._frontPaths.count()); index > unit::ItemIndex::zero();) {
        paths.insert(unit::ItemIndex::zero(), changes._frontPaths.getRef(--index));
    }
    for (const auto &path : changes._backPaths) {
        paths.append(path);
    }
    environment.insert_or_assign(name, encode<T>(paths.text()));
}

}
