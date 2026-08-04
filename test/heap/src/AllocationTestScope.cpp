// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "AllocationTestScope.hpp"

#include <algorithm>
#include <cstdlib>
#include <limits>

#ifdef _WIN32
#include <malloc.h>
#endif

namespace erbsland::test {

thread_local bool AllocationTestScope::_isActive{};
thread_local std::size_t AllocationTestScope::_allocations{};

AllocationTestScope::AllocationTestScope() noexcept {
    _allocations = 0U;
    _isActive = true;
}

AllocationTestScope::~AllocationTestScope() {
    if (!_isFinished) {
        _isActive = false;
    }
}

auto AllocationTestScope::finish() noexcept -> std::size_t {
    _isActive = false;
    _isFinished = true;
    return _allocations;
}

void AllocationTestScope::recordAllocation() noexcept {
    if (_isActive) {
        ++_allocations;
    }
}

auto AllocationTestScope::allocate(const std::size_t size) -> void * {
    const auto actualSize = std::max<std::size_t>(size, 1U);
    while (true) {
        if (auto *pointer = std::malloc(actualSize)) {
            recordAllocation();
            return pointer;
        }
        const auto handler = std::get_new_handler();
        if (handler == nullptr) {
            throw std::bad_alloc{};
        }
        handler();
    }
}

auto AllocationTestScope::allocate(const std::size_t size, const std::align_val_t alignment) -> void * {
    const auto alignmentSize = std::max<std::size_t>(static_cast<std::size_t>(alignment), alignof(void *));
    const auto actualSize = std::max<std::size_t>(size, 1U);
    while (true) {
        auto *pointer = static_cast<void *>(nullptr);
        if (actualSize <= std::numeric_limits<std::size_t>::max() - (alignmentSize - 1U)) {
            const auto roundedSize = ((actualSize + alignmentSize - 1U) / alignmentSize) * alignmentSize;
#ifdef _WIN32
            pointer = _aligned_malloc(roundedSize, alignmentSize);
#else
            pointer = std::aligned_alloc(alignmentSize, roundedSize);
#endif
        }
        if (pointer != nullptr) {
            recordAllocation();
            return pointer;
        }
        const auto handler = std::get_new_handler();
        if (handler == nullptr) {
            throw std::bad_alloc{};
        }
        handler();
    }
}

void AllocationTestScope::deallocate(void *pointer, [[maybe_unused]] const std::align_val_t alignment) noexcept {
#ifdef _WIN32
    _aligned_free(pointer);
#else
    std::free(pointer);
#endif
}

}

void *operator new(const std::size_t size) {
    return erbsland::test::AllocationTestScope::allocate(size);
}

void *operator new[](const std::size_t size) {
    return ::operator new(size);
}

void *operator new(const std::size_t size, const std::align_val_t alignment) {
    return erbsland::test::AllocationTestScope::allocate(size, alignment);
}

void *operator new[](const std::size_t size, const std::align_val_t alignment) {
    return ::operator new(size, alignment);
}

void operator delete(void *pointer) noexcept {
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

void operator delete(void *pointer, const std::align_val_t alignment) noexcept {
    erbsland::test::AllocationTestScope::deallocate(pointer, alignment);
}

void operator delete[](void *pointer, const std::align_val_t alignment) noexcept {
    ::operator delete(pointer, alignment);
}

void operator delete(
    void *pointer, [[maybe_unused]] const std::size_t size, const std::align_val_t alignment) noexcept {
    ::operator delete(pointer, alignment);
}

void operator delete[](
    void *pointer, [[maybe_unused]] const std::size_t size, const std::align_val_t alignment) noexcept {
    ::operator delete(pointer, alignment);
}
