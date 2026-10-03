// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Selector.hpp"

#include <algorithm>

namespace erbsland::conf::syntax {

auto Selector::match(const Statement &statement) const -> std::optional<std::array<int, 6>> {
    auto score = std::array<int, 6>{};
    if (_kind) {
        if (*_kind != statement.kind()) {
            return {};
        }
        ++score[5];
    }
    if (_list) {
        if (statement.kind() != Statement::Kind::Section || *_list != statement.isListSection()) {
            return {};
        }
        ++score[5];
    }
    if (_relative) {
        if (statement.kind() != Statement::Kind::Section || *_relative != statement.isRelativeSection()) {
            return {};
        }
        ++score[5];
    }
    if (_names) {
        if (statement.kind() != Statement::Kind::Section || statement.sectionNameCount() <= *_names) {
            return {};
        }
        ++score[5];
    }
    if (_meta) {
        if (statement.kind() != Statement::Kind::Assignment || *_meta != statement.isMetaValue()) {
            return {};
        }
        ++score[5];
    }
    if (!_types.empty()) {
        if (std::ranges::find(_types, statement.valueType()) == _types.end()) {
            return {};
        }
        ++score[5];
    }
    if (!_paths.empty()) {
        if (!statement.namePath()) {
            return {};
        }
        auto best = std::optional<std::array<int, 5>>{};
        for (const auto &path : _paths) {
            if (path.matches(*statement.namePath()) && (!best || path.specificity() > *best)) {
                best = path.specificity();
            }
        }
        if (!best) {
            return {};
        }
        std::copy(best->begin(), best->end(), score.begin());
    }
    return score;
}

auto Selector::setKind(Statement::Kind value) -> Selector & {
    _kind = value;
    return *this;
}

auto Selector::setListSection(bool value) -> Selector & {
    _list = value;
    return *this;
}

auto Selector::setRelativeSection(bool value) -> Selector & {
    _relative = value;
    return *this;
}

auto Selector::setMinimumSectionNames(std::size_t value) -> Selector & {
    _names = value;
    return *this;
}

auto Selector::setMetaValue(bool value) -> Selector & {
    _meta = value;
    return *this;
}

auto Selector::setValueTypes(std::vector<ValueType> values) -> Selector & {
    _types = std::move(values);
    return *this;
}

auto Selector::addPath(const text::String &pattern) -> Selector & {
    _paths.emplace_back(pattern);
    return *this;
}

}
