// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "AllocationTracker.hpp"

#include <algorithm>
#include <cstdlib>
#include <new>

#ifdef _WIN32
#include <malloc.h>
#endif

namespace app::charset {

thread_local bool AllocationTracker::_isActive{};
thread_local AllocationMeasurement AllocationTracker::_measurement{};

void AllocationTracker::start() noexcept {
    _measurement = {};
    _isActive = true;
}

auto AllocationTracker::finish() noexcept -> AllocationMeasurement {
    _isActive = false;
    return _measurement;
}

void AllocationTracker::recordAllocation(const std::size_t size) noexcept {
    if (_isActive) {
        ++_measurement.allocations;
        _measurement.allocatedBytes += size;
    }
}

void AllocationTracker::recordDeallocation() noexcept {
    if (_isActive) {
        ++_measurement.deallocations;
    }
}

}

void *operator new(const std::size_t size) {
    const auto actualSize = std::max<std::size_t>(size, 1U);
    if (auto *pointer = std::malloc(actualSize)) {
        app::charset::AllocationTracker::recordAllocation(size);
        return pointer;
    }
    throw std::bad_alloc{};
}

void *operator new[](const std::size_t size) {
    return ::operator new(size);
}

void *operator new(const std::size_t size, const std::align_val_t alignment) {
    const auto alignmentSize = std::max<std::size_t>(static_cast<std::size_t>(alignment), alignof(void *));
    const auto actualSize = std::max<std::size_t>(size, 1U);
    const auto roundedSize = ((actualSize + alignmentSize - 1U) / alignmentSize) * alignmentSize;
#ifdef _WIN32
    auto *pointer = _aligned_malloc(roundedSize, alignmentSize);
#else
    auto *pointer = std::aligned_alloc(alignmentSize, roundedSize);
#endif
    if (pointer != nullptr) {
        app::charset::AllocationTracker::recordAllocation(size);
        return pointer;
    }
    throw std::bad_alloc{};
}

void *operator new[](const std::size_t size, const std::align_val_t alignment) {
    return ::operator new(size, alignment);
}

void operator delete(void *pointer) noexcept {
    app::charset::AllocationTracker::recordDeallocation();
    std::free(pointer);
}

void operator delete[](void *pointer) noexcept {
    ::operator delete(pointer);
}

void operator delete(void *pointer, [[maybe_unused]] const std::size_t size) noexcept {
    ::operator delete(pointer);
}

void operator delete[](void *pointer, [[maybe_unused]] const std::size_t size) noexcept {
    ::operator delete(pointer);
}

void operator delete(void *pointer, [[maybe_unused]] const std::align_val_t alignment) noexcept {
#ifdef _WIN32
    app::charset::AllocationTracker::recordDeallocation();
    _aligned_free(pointer);
#else
    ::operator delete(pointer);
#endif
}

void operator delete[](void *pointer, [[maybe_unused]] const std::align_val_t alignment) noexcept {
    ::operator delete(pointer, alignment);
}

void operator delete(
    void *pointer,
    [[maybe_unused]] const std::size_t size,
    [[maybe_unused]] const std::align_val_t alignment) noexcept {
    ::operator delete(pointer, alignment);
}

void operator delete[](
    void *pointer,
    [[maybe_unused]] const std::size_t size,
    [[maybe_unused]] const std::align_val_t alignment) noexcept {
    ::operator delete(pointer, alignment);
}
