// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/mem/UnsafeCharPtr.hpp>
#include <erbsland/mem/UnsafeMemoryPtr.hpp>

#include <array>
#include <cstring>
#include <span>

namespace demo {

/// Mark and contain operations that cannot express memory bounds in their type.
///
/// The `Unsafe...Ptr` aliases behave exactly like raw pointers; their names add
/// no runtime checks. They make a low-level boundary visible in signatures so
/// reviewers can verify lifetime, bounds, overlap, and termination assumptions.
void copyBytes(
    const el::mem::UnsafeMemoryPtr destination, const el::mem::UnsafeConstMemoryPtr source, const std::size_t size) {
    std::memcpy(destination, source, size);
}

void terminateText(const el::mem::UnsafeCharPtr destination, const std::size_t index) {
    destination[index] = '\0';
}

void markedBoundary() {
    const auto source = std::array{'f', 'o', 'r', 'm'};
    auto destination = std::array<char, 5>{};

    // Safe containers establish the sizes before their pointers cross the boundary.
    const auto sourceView = std::span{source};
    auto destinationView = std::span{destination};
    copyBytes(destinationView.data(), sourceView.data(), sourceView.size_bytes());
    terminateText(destinationView.data(), sourceView.size());

    el::io::printLine("Copied concept    : "_el, destination.data());
}

}
