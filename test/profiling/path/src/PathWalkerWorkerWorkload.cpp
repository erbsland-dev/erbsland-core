// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "PathWalkerWorkerWorkload.hpp"

#include <erbsland/path/PathInfo.hpp>
#include <erbsland/path/PathWalker.hpp>
#include <erbsland/profiling/WorkerExecutionContext.hpp>

#include <filesystem>
#include <utility>

namespace app::path {

namespace pf = erbsland::profiling;

using namespace el::text::literals;

PathWalkerWorkerWorkload::PathWalkerWorkerWorkload(const PathWalkMethod method, el::Path root) :
    _method{method}, _root{std::move(root)} {
}

auto PathWalkerWorkerWorkload::execute(const pf::WorkerExecutionContext &context) -> pf::WorkerMeasurement {
    auto operations = std::uint64_t{};
    auto entries = std::uint64_t{};
    for (; operations < context.operations && !context.stopToken.stop_requested(); ++operations) {
        switch (_method) {
        case PathWalkMethod::PathCallback:
            entries += walkPathCallback();
            break;
        case PathWalkMethod::PathInfoCallback:
            entries += walkPathInfoCallback();
            break;
        case PathWalkMethod::StdRecursive:
            entries += walkStdRecursive();
            break;
        }
    }
    return pf::WorkerMeasurement{
        .operations = operations, .metrics = el::List<std::uint64_t>{entries}, .sink = entries};
}

auto PathWalkerWorkerWorkload::walkPathCallback() const -> std::uint64_t {
    auto entries = std::uint64_t{};
    auto options = el::PathWalkOptions{};
    options.setTypes(el::PathType::All).setSymlinkMode(el::SymlinkMode::Use);
    const auto result = _root.walker().walkOrThrow(
        [&](const el::Path &) -> el::PathWalkStatus {
            ++entries;
            return el::PathWalkStatus::Continue;
        },
        options);
    if (!result.isSuccessful()) {
        throw el::ApplicationError{"PathWalker path-callback traversal failed."_el};
    }
    return entries;
}

auto PathWalkerWorkerWorkload::walkPathInfoCallback() const -> std::uint64_t {
    auto entries = std::uint64_t{};
    auto options = el::PathWalkOptions{};
    options.setTypes(el::PathType::All).setSymlinkMode(el::SymlinkMode::Use);
    const auto result = _root.walker().walkOrThrow(
        [&](const el::Path &, const el::PathInfo &info) -> el::PathWalkStatus {
            static_cast<void>(info.type());
            ++entries;
            return el::PathWalkStatus::Continue;
        },
        options);
    if (!result.isSuccessful()) {
        throw el::ApplicationError{"PathWalker information-callback traversal failed."_el};
    }
    return entries;
}

auto PathWalkerWorkerWorkload::walkStdRecursive() const -> std::uint64_t {
    auto entries = std::uint64_t{1U};
    auto error = std::error_code{};
    const auto options = std::filesystem::directory_options::skip_permission_denied;
    for (
        auto iterator = std::filesystem::recursive_directory_iterator{_root.toStdPath(), options, error};
        iterator != std::filesystem::recursive_directory_iterator{};
        iterator.increment(error)) {
        if (error) {
            throw el::ApplicationError{"Standard-library recursive traversal failed."_el};
        }
        ++entries;
    }
    if (error) {
        throw el::ApplicationError{"Standard-library recursive traversal failed."_el};
    }
    return entries;
}

}
