// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../ProfileTypes.hpp"

#include <initializer_list>

namespace app::string::impl {

using namespace el::text::literals;

/// Append benchmark descriptors for a family of string API variants.
void addDescriptor(
    std::vector<UseCaseDescriptor> &target,
    const StringType type,
    const UseCase useCase,
    const std::initializer_list<el::StringLiteral> variants,
    const std::initializer_list<el::StringLiteral> apiFamilies,
    const WorkUnit workUnit = WorkUnit::CodePoints,
    const bool u8Only = false) {
    for (const auto variant : variants) {
        auto descriptor = UseCaseDescriptor{
            .type = type, .useCase = useCase, .variant = el::String{variant}, .workUnit = workUnit, .u8Only = u8Only};
        for (const auto api : apiFamilies) {
            descriptor.apiFamilies.emplace_back(api);
        }
        target.emplace_back(std::move(descriptor));
    }
}

/// Append descriptors for string inspection, traversal, and search operations.
void addReadableDescriptors(std::vector<UseCaseDescriptor> &target, const StringType type) {
    addDescriptor(
        target,
        type,
        UseCase::Inspect,
        {"character-length"_el, "display-width"_el, "validate"_el},
        {"characterLength/displayWidth/isValidUtf*"_el});
    addDescriptor(
        target,
        type,
        UseCase::ReadIndexed,
        {"native-sequential"_el, "code-point-sequential"_el, "code-point-random"_el},
        {"charAt/operator[]"_el});
    addDescriptor(target, type, UseCase::ReadIndexed, {"index-at"_el}, {"indexAt"_el});
    addDescriptor(target, type, UseCase::ReadIndexed, {"to-char-index"_el}, {"toCharIndex"_el});
    addDescriptor(
        target,
        type,
        UseCase::Traverse,
        {"reader-forward"_el, "reader-backward"_el, "iterator"_el, "for-each"_el},
        {"readCharAndAdvance/readCharAndRetreat/begin/end/forEach"_el});
    addDescriptor(
        target,
        type,
        UseCase::Compare,
        {"equal"_el, "different-front"_el, "different-back"_el, "case-folded"_el},
        {"compare"_el});
    addDescriptor(target, type, UseCase::Hash, {"normal"_el, "case-folded"_el}, {"toHash/toHashCI"_el});
    addDescriptor(
        target,
        type,
        UseCase::Search,
        {"find-hit-front"_el, "find-hit-back"_el, "find-miss"_el, "find-adversarial"_el},
        {"find"_el});
    addDescriptor(target, type, UseCase::Search, {"starts-with-hit"_el}, {"startsWith"_el});
    addDescriptor(target, type, UseCase::Search, {"ends-with-hit"_el}, {"endsWith"_el});
    addDescriptor(target, type, UseCase::Search, {"contains-hit-back"_el}, {"contains"_el});
    addDescriptor(target, type, UseCase::Search, {"count-many"_el}, {"count"_el});
    addDescriptor(target, type, UseCase::Search, {"find-first-of"_el}, {"findFirstOf"_el});
    addDescriptor(target, type, UseCase::Search, {"find-last-of"_el}, {"findLastOf"_el});
    addDescriptor(
        target,
        type,
        UseCase::Transform,
        {"unchanged"_el, "changed"_el, "truncate"_el, "align"_el},
        {"transformed/truncated/aligned"_el});
    addDescriptor(target, type, UseCase::EscapeSafe, {"escaped-size"_el}, {"escapedSize"_el});
    addDescriptor(target, type, UseCase::EscapeSafe, {"to-escaped"_el}, {"toEscaped"_el});
    addDescriptor(target, type, UseCase::EscapeSafe, {"safe-string"_el}, {"toSafeString"_el});
}

/// Append descriptors for string creation, copying, conversion, and joining.
void addLifecycleDescriptors(std::vector<UseCaseDescriptor> &target, const StringType type) {
    addDescriptor(
        target, type, UseCase::Create, {"copy-source"_el, "from-character"_el}, {"constructors/fromCharacter"_el});
    addDescriptor(
        target, type, UseCase::Copy, {"shallow"_el}, {"copy constructor/assignment"_el}, WorkUnit::Operations);
    if (type == StringType::String) {
        addDescriptor(target, type, UseCase::Copy, {"compact"_el}, {"copy"_el});
    }
    addDescriptor(
        target, type, UseCase::Move, {"ownership"_el}, {"move constructor/assignment"_el}, WorkUnit::Operations);
    addDescriptor(
        target,
        type,
        UseCase::TypeConvert,
        {type == StringType::String ? "string-to-editor"_el : "editor-to-string"_el},
        {"String/StringEditor conversion"_el},
        WorkUnit::Operations);
    addDescriptor(target, type, UseCase::WidthConvert, {"to-u8"_el, "to-u16"_el, "to-u32"_el}, {"StringConverter"_el});
    addDescriptor(
        target,
        type,
        UseCase::SliceSplit,
        {"native-middle"_el, "code-point-middle"_el, "code-point-back"_el, "split-middle"_el},
        {"slice/splitAt"_el},
        WorkUnit::Operations);
    addDescriptor(target, type, UseCase::Join, {"four-parts"_el}, {"fromJoined"_el});
}

/// Append descriptors specific to mutable string-editor operations.
void addEditorDescriptors(std::vector<UseCaseDescriptor> &target) {
    constexpr auto type = StringType::StringEditor;
    addDescriptor(
        target,
        type,
        UseCase::Storage,
        {"clear"_el, "reset"_el, "reserve"_el, "shrink"_el, "detach-shared"_el, "slice-shrink"_el},
        {"clear/reset/reserve/shrinkToFit/detach/capacity/memoryUsage"_el});
    addDescriptor(
        target,
        type,
        UseCase::Append,
        {"character-growing"_el, "text-growing"_el, "text-reserved"_el, "aliased"_el},
        {"append"_el});
    addDescriptor(
        target,
        type,
        UseCase::Insert,
        {"native-front"_el, "native-middle"_el, "native-back"_el, "code-point-middle"_el, "aliased"_el},
        {"insert"_el});
    addDescriptor(
        target,
        type,
        UseCase::Replace,
        {"native-equal"_el, "native-grow"_el, "native-shrink"_el, "code-point-middle"_el, "aliased"_el, "all-dense"_el},
        {"replace/replaceAll"_el});
    addDescriptor(
        target,
        type,
        UseCase::RemoveKeep,
        {"remove-front"_el, "remove-middle"_el, "keep-middle"_el, "all-dense"_el},
        {"remove/keep/removeAll"_el});
    addDescriptor(target, type, UseCase::Trim, {"whitespace"_el, "character-set"_el}, {"trim"_el});
    addDescriptor(target, type, UseCase::Truncate, {"end"_el, "middle"_el, "ellipsis"_el}, {"truncate"_el});
    addDescriptor(
        target, type, UseCase::CowStress, {"fanout"_el, "sliced-fanout"_el}, {"copy/slice/detach/mutation"_el});
    addDescriptor(
        target, type, UseCase::EditStress, {"growth"_el, "overlap"_el}, {"append/insert/replace/remove/truncate"_el});
}

template <typename Enum, typename ToString>
/// Parse an enum value by comparing its textual representations.
[[nodiscard]] auto parseEnum(const el::String &value, const Enum first, const Enum last, ToString toStringFn)
    -> std::optional<Enum> {
    for (auto item = first; item <= last; item = static_cast<Enum>(static_cast<int>(item) + 1)) {
        if (value == toStringFn(item)) {
            return item;
        }
    }
    return std::nullopt;
}

}
