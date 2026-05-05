// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Database.hpp"

#include "Texts.hpp"
#include "Zones.hpp"

#include "../../../text/CharSet.hpp"
#include "../../../text/Literals.hpp"
#include "../../../text/StringBuilder.hpp"
#include "../../../text/StringViewList.hpp"

#include <mutex>

namespace erbsland::time::tz::impl {

using namespace text::literals;

auto Database::zoneNameToString(const ZoneName &zoneName) -> text::String {
    auto builder = text::StringBuilder{};
    builder.append(text::String{Database::textFromIndex(zoneName.text1)});
    if (zoneName.text2 != cEmptyTextId) {
        builder.append("/"_el);
        builder.append(text::String{Database::textFromIndex(zoneName.text2)});
    }
    if (zoneName.text3 != cEmptyTextId) {
        builder.append("/"_el);
        builder.append(text::String{Database::textFromIndex(zoneName.text3)});
    }
    return builder.takeU8String();
}

auto Database::names() const -> text::StringList {
    auto result = text::StringList{};
    for (const auto &zoneName : zoneNames()) {
        result.append(zoneNameToString(zoneName));
    }
    return result;
}

auto Database::hasName(text::StringView zoneName) const noexcept -> bool {
    return zoneIdFromName(zoneName) != cZoneIdNotFound;
}

auto Database::zoneIdFromName(text::StringView zoneName) const noexcept -> ZoneId {
    if (zoneName.isEmpty()) {
        return cZoneIdNotFound;
    }
    const auto parts = text::StringViewList::fromSplit(zoneName, text::CharSet{U'/'}, unit::ElementCount{3U}, true);
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

auto Database::nameFromZoneId(ZoneId zoneId) const -> text::String {
    if (zoneId == cUtcZoneId || zoneId - 1U >= primaryZoneNames().size()) {
        return {};
    }
    return zoneNameToString(primaryZoneNames()[zoneId - 1U]);
}

auto Database::abbreviation(ZoneId zoneId, AbbreviationOffset abbreviationOffset) const -> text::String {
    const auto zoneInfo = info(zoneId);
    if (zoneInfo == nullptr) {
        return {};
    }
    const auto textId = zoneInfo->textIdFromAbbreviationOffset(abbreviationOffset);
    return text::String{textFromIndex(textId)};
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

auto Database::indexFromText(text::StringView textView) noexcept -> TextId {
    if (textView.isEmpty()) {
        return cEmptyTextId;
    }
    for (auto i = std::size_t{1}; i < textEntries().size(); ++i) {
        if (textView == text::String{textFromIndex(static_cast<TextId>(i))}) {
            return static_cast<TextId>(i);
        }
    }
    return cEmptyTextId;
}

}
