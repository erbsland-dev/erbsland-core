// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

#include <array>

namespace demo {

/// Build text in place when an editor is already the natural working value.
/// Calculate the exact native UTF-8 byte length first and reserve it once, so
/// the append operations do not repeatedly grow and copy the existing text.
auto joinWithStringEditor() -> el::String {
    const auto fragments = std::array{
        el::String{"元素: "_el},
        el::String{"金"_el},
        el::String{" ("_el},
        el::String{"Au"_el},
        el::String{"), 原子番号 "_el},
        el::String{"79"_el},
    };

    auto finalLength = el::ByteLength{};
    for (const auto &fragment : fragments) {
        finalLength += fragment.length();
    }

    auto result = el::StringEditor{};
    result.reserve(finalLength);
    for (const auto &fragment : fragments) {
        result.append(fragment);
    }
    return el::String{result};
}

/// Join a fixed set of fragments with one final allocation. `fromJoined()`
/// calculates the complete native size before it copies any character data.
auto joinWithFromJoined() -> el::String {
    return el::String::fromJoined({"元素: "_el, "金"_el, " ("_el, "Au"_el, "), 原子番号 "_el, "79"_el});
}

/// Collect a dynamic number of lightweight shared strings, then join them.
/// Growing the list can move string handles, while `join()` calculates the
/// final character-storage size once before producing the result.
auto joinWithStringList() -> el::String {
    const auto discoveredFragments = std::array{
        el::String{"元素: "_el},
        el::String{"金"_el},
        el::String{" ("_el},
        el::String{"Au"_el},
        el::String{"), 原子番号 "_el},
        el::String{"79"_el},
    };

    auto fragments = el::StringList{};
    for (const auto &fragment : discoveredFragments) {
        fragments.append(fragment);
    }
    return fragments.join();
}

/// Build through a width-independent interface and append formatted values
/// directly. The same helper can target UTF-8, UTF-16, or UTF-32 without first
/// creating a temporary string for the integer.
void appendElementLabel(el::AnyStringBuilder &builder) {
    builder.append(u"元素: "_el)
        .append(U'金')
        .append(" ("_el)
        .append("Au"_el)
        .append("), 原子番号 "_el)
        .appendInteger(79);
}

auto joinWithAnyStringBuilder() -> el::String {
    auto builder = el::AnyStringBuilder::u8();
    appendElementLabel(builder);
    return builder.takeString();
}

/// Compare four construction strategies that produce the same text. The right
/// choice depends on whether the fragment count is fixed, discovered at run
/// time, already being edited, or requires width-independent formatted output.
void joiningStrategies() {
    el::io::printLine("StringEditor ......: "_el, joinWithStringEditor());
    el::io::printLine("String::fromJoined : "_el, joinWithFromJoined());
    el::io::printLine("StringList::join ..: "_el, joinWithStringList());
    el::io::printLine("AnyStringBuilder ..: "_el, joinWithAnyStringBuilder());
}

}
