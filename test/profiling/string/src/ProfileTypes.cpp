// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ProfileTypes.hpp"

#include "impl/ProfileTypeTools.hpp"

#include <array>

namespace app::string {

using namespace el::text::literals;

auto coverageRegistry() -> const std::vector<UseCaseDescriptor> & {
    static const auto result = []() {
        auto target = std::vector<UseCaseDescriptor>{};
        for (const auto type : {StringType::String, StringType::StringEditor}) {
            impl::addLifecycleDescriptors(target, type);
            impl::addReadableDescriptors(target, type);
        }
        impl::addEditorDescriptors(target);
        impl::addDescriptor(
            target,
            StringType::String,
            UseCase::SensitiveStorage,
            {"mark"_el, "copy-release"_el},
            {"markAsSensitive/isSensitive/copy/destruction"_el},
            WorkUnit::NativeBytes,
            true);
        impl::addDescriptor(
            target,
            StringType::StringEditor,
            UseCase::SensitiveStorage,
            {"mark"_el, "detach"_el, "reset-release"_el},
            {"markAsSensitive/isSensitive/detach/reset/destruction"_el},
            WorkUnit::NativeBytes,
            true);
        return target;
    }();
    return result;
}

auto findDescriptor(const StringType type, const UseCase useCase, const el::String &variant)
    -> const UseCaseDescriptor * {
    for (const auto &descriptor : coverageRegistry()) {
        if (descriptor.type == type && descriptor.useCase == useCase && descriptor.variant == variant) {
            return &descriptor;
        }
    }
    return nullptr;
}

auto toString(const RunMode value) -> el::String {
    return value == RunMode::Profile ? "profile"_el : "benchmark"_el;
}

auto toString(const StringWidth value) -> el::String {
    constexpr auto names = std::array{"u8"_el, "u16"_el, "u32"_el};
    return names[static_cast<std::size_t>(value)];
}

auto toString(const StringType value) -> el::String {
    constexpr auto names = std::array{"string"_el, "string-editor"_el};
    return names[static_cast<std::size_t>(value)];
}

auto toString(const UseCase value) -> el::String {
    constexpr auto names = std::array{
        "create"_el,
        "copy"_el,
        "move"_el,
        "type-convert"_el,
        "width-convert"_el,
        "slice-split"_el,
        "storage"_el,
        "inspect"_el,
        "read-indexed"_el,
        "traverse"_el,
        "compare"_el,
        "hash"_el,
        "search"_el,
        "append"_el,
        "insert"_el,
        "replace"_el,
        "remove-keep"_el,
        "trim"_el,
        "truncate"_el,
        "transform"_el,
        "escape-safe"_el,
        "join"_el,
        "cow-stress"_el,
        "edit-stress"_el,
        "sensitive-storage"_el};
    return names[static_cast<std::size_t>(value)];
}

auto toString(const ContentProfile value) -> el::String {
    constexpr auto names =
        std::array{"ascii"_el, "mixed"_el, "supplementary"_el, "malformed-sparse"_el, "malformed-dense"_el};
    return names[static_cast<std::size_t>(value)];
}

auto toString(const SizeMode value) -> el::String {
    constexpr auto names = std::array{"fixed"_el, "incrementing"_el, "exponential"_el, "random"_el, "boundaries"_el};
    return names[static_cast<std::size_t>(value)];
}

auto toString(const SensitiveMode value) -> el::String {
    constexpr auto names = std::array{"not-applicable"_el, "normal"_el, "sensitive"_el};
    return names[static_cast<std::size_t>(value)];
}

auto toString(const SensitiveSelection value) -> el::String {
    constexpr auto names = std::array{"normal"_el, "sensitive"_el, "all"_el};
    return names[static_cast<std::size_t>(value)];
}

auto parseStringWidth(const el::String &value) -> std::optional<StringWidth> {
    return impl::parseEnum(value, StringWidth::U8, StringWidth::U32, [](const auto item) { return toString(item); });
}

auto parseStringType(const el::String &value) -> std::optional<StringType> {
    return impl::parseEnum(
        value, StringType::String, StringType::StringEditor, [](const auto item) { return toString(item); });
}

auto parseUseCase(const el::String &value) -> std::optional<UseCase> {
    return impl::parseEnum(
        value, UseCase::Create, UseCase::SensitiveStorage, [](const auto item) { return toString(item); });
}

auto parseContentProfile(const el::String &value) -> std::optional<ContentProfile> {
    return impl::parseEnum(
        value, ContentProfile::Ascii, ContentProfile::MalformedDense, [](const auto item) { return toString(item); });
}

auto parseSizeMode(const el::String &value) -> std::optional<SizeMode> {
    return impl::parseEnum(
        value, SizeMode::Fixed, SizeMode::Boundaries, [](const auto item) { return toString(item); });
}

auto parseSensitiveSelection(const el::String &value) -> std::optional<SensitiveSelection> {
    return impl::parseEnum(
        value, SensitiveSelection::Normal, SensitiveSelection::All, [](const auto item) { return toString(item); });
}

auto nativeBytesFor(const StringWidth width, const ContentProfile profile, const std::uint64_t codePoints) noexcept
    -> std::uint64_t {
    const auto nativeUnitBytes = width == StringWidth::U8 ? 1U : width == StringWidth::U16 ? 2U : 4U;
    if (profile == ContentProfile::Ascii || profile == ContentProfile::MalformedSparse ||
        profile == ContentProfile::MalformedDense) {
        return codePoints * nativeUnitBytes;
    }
    if (profile == ContentProfile::Supplementary) {
        return codePoints * 4U;
    }
    const auto blocks = codePoints / 100U;
    const auto remainder = codePoints % 100U;
    auto result = blocks * (width == StringWidth::U8 ? 130U : width == StringWidth::U16 ? 204U : 400U);
    for (auto index = std::uint64_t{}; index < remainder; ++index) {
        if (width == StringWidth::U8) {
            result += index < 80U ? 1U : index < 92U ? 2U : index < 98U ? 3U : 4U;
        } else if (width == StringWidth::U16) {
            result += index < 98U ? 2U : 4U;
        } else {
            result += 4U;
        }
    }
    return result;
}

}
