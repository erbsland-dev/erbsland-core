// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../ProfileTypes.hpp"

#include <array>
#include <initializer_list>

namespace app::byte {

using namespace el::text::literals;

namespace impl {

void addDescriptor(
    std::vector<UseCaseDescriptor> &target,
    const ByteType type,
    const UseCase useCase,
    const std::initializer_list<el::StringLiteral> variants,
    const std::initializer_list<el::StringLiteral> apiFamilies,
    const WorkUnit workUnit = WorkUnit::Bytes) {
    for (const auto variant : variants) {
        auto descriptor = UseCaseDescriptor{.type = type, .useCase = useCase, .variant = el::String{variant}};
        descriptor.workUnit = workUnit;
        for (const auto api : apiFamilies) {
            descriptor.apiFamilies.emplace_back(api);
        }
        target.emplace_back(std::move(descriptor));
    }
}

void addReadableDescriptors(std::vector<UseCaseDescriptor> &target, const ByteType type) {
    addDescriptor(target, type, UseCase::ReadIndexed, {"sequential"_el, "random"_el}, {"get/getOrThrow"_el});
    addDescriptor(target, type, UseCase::Traverse, {"for-each"_el, "span"_el}, {"forEach/span"_el});
    addDescriptor(target, type, UseCase::IntegerRead, {"little"_el, "big"_el}, {"getInteger/getIntegerInto"_el});
}

void addMutableDescriptors(std::vector<UseCaseDescriptor> &target, const ByteType type) {
    addDescriptor(target, type, UseCase::WriteIndexed, {"set"_el, "xor-at"_el}, {"set/xorAt"_el});
    addDescriptor(target, type, UseCase::IntegerWrite, {"little"_el, "big"_el}, {"setInteger"_el});
    addDescriptor(target, type, UseCase::Fill, {"whole"_el, "range"_el}, {"fill"_el});
    addDescriptor(target, type, UseCase::Overwrite, {"external"_el, "aliased"_el}, {"overwrite"_el});
    addDescriptor(target, type, UseCase::Xor, {"whole"_el, "range"_el}, {"xorWith"_el});
}

void addDynamicDescriptors(std::vector<UseCaseDescriptor> &target, const ByteType type) {
    addDescriptor(target, type, UseCase::ClearReset, {"clear"_el, "reset"_el}, {"clear/reset"_el});
    addDescriptor(target, type, UseCase::ReserveShrink, {"reserve"_el, "shrink"_el}, {"reserve/shrinkToFit"_el});
    addDescriptor(target, type, UseCase::Resize, {"grow"_el, "shrink"_el}, {"resize"_el});
    addDescriptor(
        target, type, UseCase::Append, {"byte-growing"_el, "span-growing"_el, "span-reserved"_el}, {"append"_el});
    addDescriptor(target, type, UseCase::Insert, {"front"_el, "middle"_el, "back"_el, "aliased"_el}, {"insert"_el});
    addDescriptor(
        target, type, UseCase::Replace, {"equal"_el, "grow"_el, "shrink"_el, "aliased"_el}, {"replace/replaced"_el});
    addDescriptor(
        target, type, UseCase::RemoveKeep, {"front"_el, "middle"_el, "back"_el, "keep"_el}, {"remove/removed/keep"_el});
    addDescriptor(
        target, type, UseCase::EditStress, {"growth"_el, "overlap"_el}, {"append/insert/replace/remove/resize"_el});
}

}

}
