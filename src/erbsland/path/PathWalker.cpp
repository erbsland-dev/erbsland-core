// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PathWalker.hpp"

#include "Path.hpp"
#include "PathError.hpp"

#include "impl/PathWalker.hpp"

#include "../err/Exception.hpp"
#include "../text/Literals.hpp"

#include <memory>

namespace erbsland::path {

using namespace text::literals;

PathWalker::PathWalker() = default;

PathWalker::~PathWalker() {
}

PathWalker::PathWalker(PathWalker &&) noexcept = default;

auto PathWalker::operator=(PathWalker &&) noexcept -> PathWalker & = default;

PathWalker::PathWalker(const Path &path) {
    if (!path.isEmpty()) {
        _impl = std::make_unique<impl::PathWalker>(path);
    }
}

auto PathWalker::isEmpty() const -> bool {
    return _impl == nullptr;
}

auto PathWalker::path() const -> const Path & {
    return _impl == nullptr ? Path::empty() : _impl->path();
}

auto PathWalker::walk(const PathWalkFn &walkFn, const PathWalkOptions options) const -> PathWalkResult {
    try {
        return walkOrThrow(walkFn, options);
    } catch (const err::Exception &) {
        return PathWalkResult::Failure;
    }
}

auto PathWalker::walk(const PathInfoWalkFn &walkFn, const PathWalkOptions options) const -> PathWalkResult {
    try {
        return walkOrThrow(walkFn, options);
    } catch (const err::Exception &) {
        return PathWalkResult::Failure;
    }
}

auto PathWalker::walkOrThrow(const PathWalkFn &walkFn, const PathWalkOptions options) const -> PathWalkResult {
    if (isEmpty()) {
        throw PathError{"Path walk has no base path"_el};
    }
    return _impl->walkOrThrow(walkFn, options);
}

auto PathWalker::walkOrThrow(const PathInfoWalkFn &walkFn, const PathWalkOptions options) const -> PathWalkResult {
    if (isEmpty()) {
        throw PathError{"Path walk has no base path"_el};
    }
    return _impl->walkOrThrow(walkFn, options);
}

}
