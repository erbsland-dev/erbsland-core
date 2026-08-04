// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "NamePath.hpp"

#include "impl/lexer/NameLexer.hpp"

#include "../text/StringEditor.hpp"

#include <algorithm>
#include <utility>

namespace erbsland::conf {

using namespace text::literals;

auto NamePath::empty() const noexcept -> bool {
    return _names.empty();
}

auto NamePath::size() const noexcept -> Count {
    return _names.size();
}

auto NamePath::at(const Index index) const -> const Name & {
    return _names.at(index);
}

auto NamePath::find(const Name &name) const noexcept -> Index {
    if (const auto it = std::ranges::find(_names, name); it != _names.end()) {
        return static_cast<Index>(std::distance(_names.begin(), it));
    }
    return npos;
}

auto NamePath::front() const -> const Name & {
    if (_names.empty()) {
        return Name::emptyInstance();
    }
    return _names.front();
}

auto NamePath::back() const -> const Name & {
    if (_names.empty()) {
        return Name::emptyInstance();
    }
    return _names.back();
}

auto NamePath::containsIndex() const noexcept -> bool {
    return std::any_of(
        _names.begin(), _names.end(), [](const auto &name) -> bool { return name.isIndex() || name.isTextIndex(); });
}

auto NamePath::containsText() const noexcept -> bool {
    return std::any_of(_names.begin(), _names.end(), [](const auto &name) -> bool { return name.isText(); });
}

auto NamePath::begin() const noexcept -> NameList::const_iterator {
    return _names.begin();
}

auto NamePath::end() const noexcept -> NameList::const_iterator {
    return _names.end();
}

auto NamePath::view() const noexcept -> std::span<const Name> {
    return std::span{_names};
}

auto NamePath::parent() const noexcept -> NamePath {
    if (size() <= 1) {
        return NamePath{};
    }
    return NamePath(_names.begin(), _names.end() - 1);
}

auto NamePath::subPath(const Index pos, const Count count) const noexcept -> NamePath {
    if (pos > size()) {
        return {};
    }
    const auto endIndex = (count == npos) ? size() : std::min(size(), pos + count);
    return NamePath(
        _names.begin() + static_cast<difference_type>(pos), _names.begin() + static_cast<difference_type>(endIndex));
}

void NamePath::append(const NamePath &namePath) noexcept {
    if (namePath.empty()) {
        return;
    }
    _names.insert(_names.end(), namePath._names.begin(), namePath._names.end());
}

void NamePath::prepend(const NamePath &namePath) noexcept {
    if (namePath.empty()) {
        return;
    }
    _names.insert(_names.begin(), namePath._names.begin(), namePath._names.end());
}

void NamePath::popBack() noexcept {
    if (_names.empty()) {
        return;
    }
    _names.pop_back();
}

void NamePath::clear() noexcept {
    _names.clear();
}

auto NamePath::toText() const noexcept -> text::String {
    text::StringEditor result;
    bool first = true;
    for (const auto &name : _names) {
        if (!first && !name.isIndex()) {
            result.append("."_el);
        }
        result.append(name.toPathText());
        first = false;
    }
    return result;
}

auto NamePath::fromText(const text::String &text) -> NamePath {
    impl::NameLexer lexer(text);
    lexer.initialize();
    NameList names;
    while (lexer.hasNext()) {
        names.push_back(lexer.next());
    }
    return NamePath{names};
}

auto toNamePath(const NamePathLike &namePathLike) -> NamePath {
    if (const auto *namePath = std::get_if<NamePath>(&namePathLike)) {
        return *namePath;
    }
    if (const auto *name = std::get_if<Name>(&namePathLike)) {
        return {*name};
    }
    if (const auto *text = std::get_if<text::String>(&namePathLike)) {
        return NamePath::fromText(*text);
    }
    return NamePath{Name::createIndex(std::get<std::size_t>(namePathLike))};
}

#ifdef ERBSLAND_CORE_CONF_INTERNAL_VIEWS
auto internalView(const NamePath &object) -> impl::InternalViewPtr {
    auto result = impl::InternalView::create();
    result->setValue("names", impl::InternalView::createList(0, object._names.begin(), object._names.end()));
    return result;
}
#endif

}
