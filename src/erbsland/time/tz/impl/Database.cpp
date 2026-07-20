// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Database.hpp"

#include "Texts.hpp"
#include "Zones.hpp"

#include "../../../text/CharSet.hpp"
#include "../../../text/Literals.hpp"
#include "../../../text/StringConverter.hpp"
#include "../../../text/StringEditor.hpp"
#include "../../../text/StringList.hpp"

#include <mutex>

namespace erbsland::time::tz::impl {

using namespace text::literals;
using namespace text;

auto Database::zoneNameToString(const ZoneName &zoneName) -> String {
    const auto secondText =
        zoneName.text2 == cEmptyTextId ? String{} : StringConverter{Database::textFromIndex(zoneName.text2)}.toString();
    const auto thirdText =
        zoneName.text3 == cEmptyTextId ? String{} : StringConverter{Database::textFromIndex(zoneName.text3)}.toString();
    return String::fromJoined(
        {StringConverter{Database::textFromIndex(zoneName.text1)}.toString(),
            zoneName.text2 == cEmptyTextId ? String{} : String{"/"_el},
            secondText,
            zoneName.text3 == cEmptyTextId ? String{} : String{"/"_el},
            thirdText});
}

auto Database::names() const -> StringList {
    auto result = StringList{};
    for (const auto &zoneName : zoneNames()) {
        result.append(zoneNameToString(zoneName));
    }
    return result;
}

auto Database::hasName(const String &zoneName) const noexcept -> bool {
    return zoneIdFromName(zoneName) != cZoneIdNotFound;
}

auto Database::zoneIdFromName(const String &zoneName) const noexcept -> ZoneId {
    static const auto separatorCharacters = CharSet{U'/'};
    if (zoneName.isEmpty()) {
        return cZoneIdNotFound;
    }
    const auto parts = StringList::fromSplit(zoneName, separatorCharacters, unit::ElementCount{3U}, true);
    if (parts.isEmpty() || parts.count() > unit::ElementCount{3U}) {
        return cZoneIdNotFound;
    }
    const auto text1 = indexFromText(parts.get(unit::ElementIndex::zero()));
    const auto text2 =
        parts.count() < unit::ElementCount{2U} ? cEmptyTextId : indexFromText(parts.get(unit::ElementIndex{1U}));
    const auto text3 =
        parts.count() < unit::ElementCount{3U} ? cEmptyTextId : indexFromText(parts.get(unit::ElementIndex{2U}));
    if (text1 == cEmptyTextId) {
        return cZoneIdNotFound;
    }
    for (const auto &entry : zoneNames()) {
        if (entry.text1 == text1 && entry.text2 == text2 && entry.text3 == text3) {
            return entry.id;
        }
    }
    return cZoneIdNotFound;
}

auto Database::nameFromZoneId(ZoneId zoneId) const -> String {
    if (zoneId == cUtcZoneId || zoneId - 1U >= primaryZoneNames().size()) {
        return {};
    }
    return zoneNameToString(primaryZoneNames()[zoneId - 1U]);
}

auto Database::abbreviation(ZoneId zoneId, AbbreviationOffset abbreviationOffset) const -> String {
    const auto zoneInfo = info(zoneId);
    if (zoneInfo == nullptr) {
        return {};
    }
    const auto textId = zoneInfo->textIdFromAbbreviationOffset(abbreviationOffset);
    return StringConverter{textFromIndex(textId)}.toString();
}

auto Database::info(ZoneId zoneId) const noexcept -> std::shared_ptr<const Info> {
    if (zoneId == cUtcZoneId || zoneId == cZoneIdNotFound) {
        return {};
    }
    {
        auto lock = std::shared_lock{_infoMutex};
        if (const auto it = _infoCache.find(zoneId); it != _infoCache.end()) {
            return it->second;
        }
    }
    auto lock = std::scoped_lock{_infoMutex};
    if (const auto it = _infoCache.find(zoneId); it != _infoCache.end()) {
        return it->second;
    }
    auto result = impl::info(zoneId);
    if (result == nullptr) {
        return {};
    }
    auto shared = std::shared_ptr<const Info>{std::move(result)};
    _infoCache.emplace(zoneId, shared);
    return shared;
}

auto Database::textFromIndex(TextId index) noexcept -> std::string_view {
    if (index == cEmptyTextId || index >= textEntries().size()) {
        return {};
    }
    const auto &text = textEntries()[index];
    return std::string_view{textBlock().data() + text.offset, text.length};
}

auto Database::indexFromText(std::string_view text) noexcept -> TextId {
    if (text.empty()) {
        return cEmptyTextId;
    }
    for (auto i = std::size_t{1}; i < textEntries().size(); ++i) {
        if (textFromIndex(static_cast<TextId>(i)) == text) {
            return static_cast<TextId>(i);
        }
    }
    return cEmptyTextId;
}

auto Database::indexFromText(const String &textView) noexcept -> TextId {
    if (textView.isEmpty()) {
        return cEmptyTextId;
    }
    for (auto i = std::size_t{1}; i < textEntries().size(); ++i) {
        if (textView == StringConverter{textFromIndex(static_cast<TextId>(i))}.toString()) {
            return static_cast<TextId>(i);
        }
    }
    return cEmptyTextId;
}

}
