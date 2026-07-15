// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TempDirectory.hpp"

#include "PathError.hpp"
#include "PathOperations.hpp"
#include "PathRemoveOptions.hpp"

#include "../text/Literals.hpp"

#include <utility>

namespace erbsland::path {

using namespace text::literals;

TempDirectory::TempDirectory(Path path, const bool removeOnDestroy) :
    _path{std::move(path)}, _removeOnDestroy{removeOnDestroy} {
}

TempDirectory::~TempDirectory() {
    if (_removeOnDestroy) {
        static_cast<void>(remove());
    }
}

auto TempDirectory::isEmpty() const noexcept -> bool {
    return _path.isEmpty();
}

auto TempDirectory::path() const noexcept -> const Path & {
    return _path;
}

auto TempDirectory::removeOnDestroy() const noexcept -> bool {
    return _removeOnDestroy;
}

void TempDirectory::setRemoveOnDestroy(const bool value) noexcept {
    _removeOnDestroy = value;
}

auto TempDirectory::release() noexcept -> Path {
    auto result = _path;
    _path = {};
    _removeOnDestroy = false;
    return result;
}

auto TempDirectory::remove() noexcept -> util::Result {
    try {
        removeOrThrow();
        return util::Result::Success;
    } catch (const PathError &) {
        return util::Result::Failure;
    }
}

void TempDirectory::removeOrThrow() {
    if (_path.isEmpty()) {
        return;
    }
    auto options = PathRemoveOptions{};
    options.setRecursive(true);
    _path.operations().removeOrThrow(options);
    _path = {};
    _removeOnDestroy = false;
}

}
