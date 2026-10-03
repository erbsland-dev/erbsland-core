// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "DisplayText.hpp"

#include "DisplayTextCore.hpp"

#include "../../err/LogicFailure.hpp"
#include "../../text/Literals.hpp"
#include "../../text/StringJoins.hpp"
#include "../../text/StringSide.hpp"

#include <algorithm>
#include <utility>

namespace erbsland::i18n::impl {

using namespace text::literals;
using text::Char;
using text::String;
using text::StringFormat;
using text::StringSide;

DisplayText::DisplayText() {
    _layers.push_back(DisplayTextCore::create());
}

auto DisplayText::text(const String &key) const -> String {
    const auto lock = std::scoped_lock{_mutex};
    return resolveText(key);
}

auto DisplayText::resolveText(const String &key) const -> String {
    if (const auto cached = _textCache.get(key); cached.has_value()) {
        return *cached;
    }
    const auto source = findSourceText(key);
    auto result = source.value_or(key);
    if (_translator != nullptr && source.has_value()) {
        auto translated = _translator->translate(key, result);
        if (!translated.isEmpty()) {
            result = std::move(translated);
        }
    }
    _textCache.set(key, result);
    return result;
}

auto DisplayText::format(const String &key) const -> StringFormat {
    const auto lock = std::scoped_lock{_mutex};
    if (const auto cached = _formatCache.get(key); cached.has_value()) {
        return **cached;
    }
    auto result = StringFormat{resolveText(key)};
    _formatCache.set(key, std::make_shared<const StringFormat>(result));
    return result;
}

auto DisplayText::translator() const -> DisplayTextTranslatorConstPtr {
    const auto lock = std::scoped_lock{_mutex};
    return _translator;
}

void DisplayText::addData(DisplayTextDataConstPtr data) {
    if (data == nullptr) {
        throw err::LogicFailure{"Display-text data must not be null."_el};
    }
    const auto lock = std::scoped_lock{_mutex};
    if (std::ranges::find(_layers, data) != _layers.end()) {
        return;
    }
    _layers.push_back(std::move(data));
    clearCaches();
}

auto DisplayText::removeData(const DisplayTextDataConstPtr &data) -> bool {
    if (data == nullptr) {
        throw err::LogicFailure{"Display-text data must not be null."_el};
    }
    const auto lock = std::scoped_lock{_mutex};
    const auto found = std::ranges::find(_layers, data);
    if (found == _layers.end() || found == _layers.begin()) {
        return false;
    }
    _layers.erase(found);
    clearCaches();
    return true;
}

void DisplayText::setTranslator(DisplayTextTranslatorConstPtr translator) {
    const auto lock = std::scoped_lock{_mutex};
    _translator = std::move(translator);
    clearCaches();
}

void DisplayText::clearCaches() {
    _textCache.clear();
    _formatCache.clear();
}

auto DisplayText::findInLayers(const String &key) const -> std::optional<String> {
    for (auto layer = _layers.rbegin(); layer != _layers.rend(); ++layer) {
        if (auto value = (*layer)->findText(key); value.has_value()) {
            return value;
        }
    }
    return {};
}

auto DisplayText::findSourceText(const String &key) const -> std::optional<String> {
    if (auto value = findInLayers(key); value.has_value()) {
        return value;
    }
    const auto middle = domainKey(key);
    if (middle != key) {
        if (auto value = findInLayers(middle); value.has_value()) {
            return value;
        }
    }
    const auto shortName = finalKey(key);
    if (shortName != middle) {
        return findInLayers(shortName);
    }
    return {};
}

auto DisplayText::domainKey(const String &key) -> String {
    const auto parts = key.splitAtFirst("."_el);
    if (!parts || !parts->second.contains("."_el)) {
        return key;
    }
    const auto lastPart = finalKey(key);
    return text::join(parts->first, "."_el, lastPart);
}

auto DisplayText::finalKey(const String &key) -> String {
    if (const auto parts = key.splitAtLast("."_el)) {
        return parts->second;
    }
    return key;
}

}
