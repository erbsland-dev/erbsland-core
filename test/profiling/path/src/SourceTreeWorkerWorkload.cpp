// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "SourceTreeWorkerWorkload.hpp"

#include "impl/SourceTreeSelection.hpp"

#include <erbsland/ApplicationError.hpp>
#include <erbsland/ByteBlock.hpp>
#include <erbsland/EncodingMode.hpp>
#include <erbsland/List.hpp>
#include <erbsland/PathContent.hpp>
#include <erbsland/PathInfo.hpp>
#include <erbsland/PathReadDataOptions.hpp>
#include <erbsland/PathReadTextOptions.hpp>
#include <erbsland/PathWalker.hpp>
#include <erbsland/PathWalkResult.hpp>
#include <erbsland/PathWalkStatus.hpp>
#include <erbsland/profiling/WorkerExecutionContext.hpp>
#include <erbsland/String.hpp>
#include <erbsland/SymlinkMode.hpp>

#include <utility>

namespace app::path {

namespace pf = erbsland::profiling;

using namespace el::text::literals;

SourceTreeWorkerWorkload::SourceTreeWorkerWorkload(const SourceReadMethod method, el::Path root) :
    _method{method}, _root{std::move(root)} {
}

auto SourceTreeWorkerWorkload::execute(const pf::WorkerExecutionContext &context) -> pf::WorkerMeasurement {
    auto operations = std::uint64_t{};
    auto total = Counters{};
    for (; operations < context.operations && !context.stopToken.stop_requested(); ++operations) {
        auto pass = Counters{};
        if (!walkAndRead(pass, context.stopToken)) {
            break;
        }
        total.entries += pass.entries;
        total.files += pass.files;
        total.returnedBytes += pass.returnedBytes;
    }
    return pf::WorkerMeasurement{
        .operations = operations,
        .metrics = el::List<std::uint64_t>{total.entries, total.files, total.returnedBytes},
        .sink = total.files ^ total.returnedBytes};
}

auto SourceTreeWorkerWorkload::walkAndRead(Counters &counters, const std::stop_token stopToken) const -> bool {
    const auto result = _root.walker().walkOrThrow(
        [&](const el::Path &path, const el::PathInfo &info) -> el::PathWalkStatus {
            if (stopToken.stop_requested()) {
                return el::PathWalkStatus::Stop;
            }
            ++counters.entries;
            if (info.isRegularFile()) {
                switch (_method) {
                case SourceReadMethod::Text:
                    counters.returnedBytes += readTextFile(path);
                    break;
                case SourceReadMethod::Data:
                    counters.returnedBytes += readDataFile(path);
                    break;
                }
                ++counters.files;
            }
            return el::PathWalkStatus::Continue;
        },
        impl::sourceTreeWalkOptions());
    if (result == el::PathWalkResult::Stopped) {
        return false;
    }
    if (!result.isSuccessful()) {
        throw el::ApplicationError{"Source-tree traversal failed."_el};
    }
    return true;
}

auto SourceTreeWorkerWorkload::readTextFile(const el::Path &path) const -> std::uint64_t {
    const auto text = path.content().readTextOrThrow(
        el::PathReadTextOptions{}.setEncodingMode(el::EncodingMode::Tolerant).setSymlinkMode(el::SymlinkMode::Follow));
    return text.length().toRawValue();
}

auto SourceTreeWorkerWorkload::readDataFile(const el::Path &path) const -> std::uint64_t {
    const auto data = path.content().readDataOrThrow(el::PathReadDataOptions{}.setSymlinkMode(el::SymlinkMode::Follow));
    return data.length().toRawValue();
}

}
