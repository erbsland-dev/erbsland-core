// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ProfileTypes.hpp"

#include "impl/ProfileTypeTools.hpp"

namespace app::byte {

using namespace el::text::literals;
using namespace impl;

auto coverageRegistry() -> const std::vector<UseCaseDescriptor> & {
    static const auto result = []() {
        auto target = std::vector<UseCaseDescriptor>{};

        addDescriptor(target, ByteType::Array, UseCase::Create, {"zeroed"_el}, {"ByteArray()"_el});
        addDescriptor(
            target,
            ByteType::Array,
            UseCase::Copy,
            {"value"_el},
            {"copy constructor/assignment"_el},
            WorkUnit::Operations);
        addDescriptor(
            target,
            ByteType::Array,
            UseCase::Move,
            {"value"_el},
            {"move constructor/assignment"_el},
            WorkUnit::Operations);
        addDescriptor(target, ByteType::Array, UseCase::Slice, {"span"_el}, {"span(range)"_el}, WorkUnit::Operations);
        addDescriptor(target, ByteType::Array, UseCase::Convert, {"buffer"_el}, {"toByteBuffer"_el});
        addReadableDescriptors(target, ByteType::Array);
        addMutableDescriptors(target, ByteType::Array);
        addDescriptor(
            target, ByteType::Array, UseCase::Compare, {"equal"_el, "different"_el}, {"comparison operators"_el});
        addDescriptor(target, ByteType::Array, UseCase::SecureErase, {"whole"_el}, {"secureErase"_el});
        addDescriptor(
            target,
            ByteType::Array,
            UseCase::Bitwise,
            {"or"_el, "and"_el, "xor"_el, "invert"_el},
            {"bitwise operators"_el});
        addDescriptor(
            target,
            ByteType::Array,
            UseCase::ShiftRotate,
            {"left"_el, "right"_el, "rotate"_el},
            {"shifted/rotated"_el});
        addDescriptor(
            target,
            ByteType::Array,
            UseCase::PerByteShiftRotate,
            {"shift"_el, "rotate"_el},
            {"eachByteShifted/Rotated"_el});

        addDescriptor(
            target, ByteType::Block, UseCase::Create, {"copy-span"_el, "filled"_el}, {"constructors/fromSpan"_el});
        addDescriptor(
            target,
            ByteType::Block,
            UseCase::Copy,
            {"shallow"_el, "shared-contention"_el},
            {"copy constructor/assignment"_el},
            WorkUnit::Operations);
        addDescriptor(
            target,
            ByteType::Block,
            UseCase::Move,
            {"ownership"_el},
            {"move constructor/assignment"_el},
            WorkUnit::Operations);
        addDescriptor(
            target,
            ByteType::Block,
            UseCase::Slice,
            {"shared"_el, "shared-slice"_el},
            {"slice/span(range)"_el},
            WorkUnit::Operations);
        addDescriptor(
            target,
            ByteType::Block,
            UseCase::Convert,
            {"buffer"_el, "vector"_el},
            {"toByteBuffer/toUInt8Vector/toCharVector"_el});
        addReadableDescriptors(target, ByteType::Block);
        addDescriptor(
            target, ByteType::Block, UseCase::Compare, {"equal"_el, "different"_el}, {"comparison operators"_el});
        addDescriptor(
            target,
            ByteType::Block,
            UseCase::PrefixSuffix,
            {"prefix"_el, "suffix"_el, "contains"_el},
            {"startsWith/endsWith/contains"_el});
        addDescriptor(
            target,
            ByteType::Block,
            UseCase::Find,
            {"begin"_el, "end"_el, "miss"_el, "adversarial"_el},
            {"find/findLast"_el});
        addDescriptor(
            target,
            ByteType::Block,
            UseCase::SecureErase,
            {"unique"_el, "shared"_el},
            {"markAsSensitive/secureErase"_el});

        addDescriptor(
            target,
            ByteType::BlockEditor,
            UseCase::Create,
            {"copy-span"_el, "filled"_el},
            {"constructors/fromSpan/fromVector"_el});
        addDescriptor(
            target,
            ByteType::BlockEditor,
            UseCase::Copy,
            {"shallow"_el},
            {"copy constructor/assignment"_el},
            WorkUnit::Operations);
        addDescriptor(
            target,
            ByteType::BlockEditor,
            UseCase::Move,
            {"ownership"_el},
            {"move constructor/assignment"_el},
            WorkUnit::Operations);
        addDescriptor(
            target,
            ByteType::BlockEditor,
            UseCase::Slice,
            {"shared"_el},
            {"slice/span(range)"_el},
            WorkUnit::Operations);
        addDescriptor(
            target,
            ByteType::BlockEditor,
            UseCase::Convert,
            {"buffer"_el, "vector"_el},
            {"toByteBuffer/toUInt8Vector/toCharVector"_el});
        addReadableDescriptors(target, ByteType::BlockEditor);
        addMutableDescriptors(target, ByteType::BlockEditor);
        addDynamicDescriptors(target, ByteType::BlockEditor);
        addDescriptor(target, ByteType::BlockEditor, UseCase::Detach, {"unique"_el, "shared"_el}, {"detach"_el});
        addDescriptor(
            target, ByteType::BlockEditor, UseCase::Compare, {"equal"_el, "different"_el}, {"comparison operators"_el});
        addDescriptor(
            target,
            ByteType::BlockEditor,
            UseCase::PrefixSuffix,
            {"prefix"_el, "suffix"_el, "contains"_el},
            {"startsWith/endsWith/contains"_el});
        addDescriptor(
            target,
            ByteType::BlockEditor,
            UseCase::Find,
            {"begin"_el, "end"_el, "miss"_el, "adversarial"_el},
            {"find/findLast"_el});
        addDescriptor(target, ByteType::BlockEditor, UseCase::Join, {"parts"_el}, {"join/fromJoined"_el});
        addDescriptor(
            target,
            ByteType::BlockEditor,
            UseCase::SecureErase,
            {"unique"_el, "shared"_el},
            {"markAsSensitive/secureErase"_el});
        addDescriptor(
            target,
            ByteType::BlockEditor,
            UseCase::CowStress,
            {"fanout"_el},
            {"ByteBlock conversion/slice/detach/mutation"_el});

        addDescriptor(
            target, ByteType::Buffer, UseCase::Create, {"copy-span"_el, "filled"_el}, {"constructors/fromSpan"_el});
        addDescriptor(target, ByteType::Buffer, UseCase::Copy, {"deep"_el}, {"copy constructor/assignment"_el});
        addDescriptor(
            target,
            ByteType::Buffer,
            UseCase::Move,
            {"ownership"_el},
            {"move constructor/assignment"_el},
            WorkUnit::Operations);
        addDescriptor(target, ByteType::Buffer, UseCase::Slice, {"span"_el}, {"span(range)"_el}, WorkUnit::Operations);
        addDescriptor(target, ByteType::Buffer, UseCase::Convert, {"vector"_el}, {"toUInt8Vector/toCharVector"_el});
        addReadableDescriptors(target, ByteType::Buffer);
        addMutableDescriptors(target, ByteType::Buffer);
        addDynamicDescriptors(target, ByteType::Buffer);
        addDescriptor(
            target, ByteType::Buffer, UseCase::Compare, {"equal"_el, "different"_el}, {"comparison operators"_el});
        addDescriptor(
            target,
            ByteType::Buffer,
            UseCase::SecureErase,
            {"whole"_el, "disable-mode"_el},
            {"setSensitive/secureErase"_el});

        addDescriptor(
            target, ByteType::RingBuffer, UseCase::Create, {"fixed"_el, "growable"_el}, {"RingBuffer constructors"_el});
        addDescriptor(
            target,
            ByteType::RingBuffer,
            UseCase::RingReserveGrow,
            {"reserve"_el, "geometric"_el, "failure"_el},
            {"canWrite/reserveAdditional"_el});
        addDescriptor(
            target,
            ByteType::RingBuffer,
            UseCase::RingTransfer,
            {"partial"_el, "exact"_el, "exact-failure"_el},
            {"write/writeExact/read"_el});
        addDescriptor(target, ByteType::RingBuffer, UseCase::RingOwnedRead, {"block"_el}, {"read(ByteLength)"_el});
        addDescriptor(
            target,
            ByteType::RingBuffer,
            UseCase::RingWrappedCycle,
            {"fixed"_el, "growable"_el},
            {"write/read wrapped spans"_el});
        addDescriptor(
            target,
            ByteType::RingBuffer,
            UseCase::RingInteger,
            {"little"_el, "big"_el},
            {"readInteger/writeInteger"_el});
        addDescriptor(
            target,
            ByteType::RingBuffer,
            UseCase::RingClearShrinkSwap,
            {"clear"_el, "shrink"_el, "swap"_el},
            {"clear/secureErase/shrinkToInitial/swap"_el});
        addDescriptor(
            target,
            ByteType::RingBuffer,
            UseCase::EditStress,
            {"producer-consumer"_el},
            {"reserve/write/read/grow/shrink"_el});
        return target;
    }();
    return result;
}

auto findDescriptor(const ByteType type, const UseCase useCase, const el::String &variant)
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

auto toString(const ByteType value) -> el::String {
    constexpr auto names =
        std::array{"byte-array"_el, "byte-block"_el, "byte-block-editor"_el, "byte-buffer"_el, "byte-ring-buffer"_el};
    return names[static_cast<std::size_t>(value)];
}

auto toString(const UseCase value) -> el::String {
    constexpr auto names = std::array{
        "create"_el,
        "copy"_el,
        "move"_el,
        "slice"_el,
        "convert"_el,
        "clear-reset"_el,
        "reserve-shrink"_el,
        "detach"_el,
        "secure-erase"_el,
        "read-indexed"_el,
        "write-indexed"_el,
        "traverse"_el,
        "integer-read"_el,
        "integer-write"_el,
        "compare"_el,
        "prefix-suffix"_el,
        "find"_el,
        "fill"_el,
        "overwrite"_el,
        "xor"_el,
        "resize"_el,
        "append"_el,
        "insert"_el,
        "replace"_el,
        "remove-keep"_el,
        "join"_el,
        "bitwise"_el,
        "shift-rotate"_el,
        "per-byte-shift-rotate"_el,
        "ring-reserve-grow"_el,
        "ring-transfer"_el,
        "ring-owned-read"_el,
        "ring-wrapped-cycle"_el,
        "ring-integer"_el,
        "ring-clear-shrink-swap"_el,
        "cow-stress"_el,
        "edit-stress"_el};
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

auto supportsSensitiveMode(const ByteType value) noexcept -> bool {
    return value != ByteType::Array;
}

auto parseByteType(const el::String &value) -> std::optional<ByteType> {
    for (
        auto type = ByteType::Array; type <= ByteType::RingBuffer;
        type = static_cast<ByteType>(static_cast<int>(type) + 1)) {
        if (value == toString(type)) {
            return type;
        }
    }
    return std::nullopt;
}

auto parseUseCase(const el::String &value) -> std::optional<UseCase> {
    for (
        auto useCase = UseCase::Create; useCase <= UseCase::EditStress;
        useCase = static_cast<UseCase>(static_cast<int>(useCase) + 1)) {
        if (value == toString(useCase)) {
            return useCase;
        }
    }
    return std::nullopt;
}

auto parseSizeMode(const el::String &value) -> std::optional<SizeMode> {
    for (
        auto mode = SizeMode::Fixed; mode <= SizeMode::Boundaries;
        mode = static_cast<SizeMode>(static_cast<int>(mode) + 1)) {
        if (value == toString(mode)) {
            return mode;
        }
    }
    return std::nullopt;
}

auto parseSensitiveSelection(const el::String &value) -> std::optional<SensitiveSelection> {
    for (
        auto mode = SensitiveSelection::Normal; mode <= SensitiveSelection::All;
        mode = static_cast<SensitiveSelection>(static_cast<int>(mode) + 1)) {
        if (value == toString(mode)) {
            return mode;
        }
    }
    return std::nullopt;
}

}
