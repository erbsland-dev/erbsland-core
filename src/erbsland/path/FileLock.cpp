// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "FileLock.hpp"

#include "PathError.hpp"
#include "PathErrorContext.hpp"

#include "impl/FileLock.hpp"

#include "../text/Literals.hpp"

#include <utility>

namespace erbsland::path {

using namespace text::literals;

auto FileLock::createLockPath(const Path &path) -> Path {
    if (!path.isValid()) {
        throw PathError{
            PathErrorContext{"File lock could not be acquired"_el, "A lock requires a valid path."_el}.setSourcePath(
                path.toString())};
    }
    return Path{text::String::fromJoined({path.toString(), ".lock"_el})};
}

FileLock::FileLock(Path path) :
    _path{std::move(path)}, _lockPath{createLockPath(_path)}, _impl{impl::FileLock::create(_path, _lockPath)} {
}

FileLock::~FileLock() = default;

FileLock::FileLock(FileLock &&other) noexcept = default;

auto FileLock::operator=(FileLock &&other) noexcept -> FileLock & = default;

auto FileLock::isLocked() const noexcept -> bool {
    return _impl != nullptr;
}

}
