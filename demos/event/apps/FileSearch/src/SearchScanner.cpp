// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "SearchScanner.hpp"

#include <erbsland/PathInfo.hpp>
#include <erbsland/PathWalker.hpp>
#include <erbsland/TimeAmounts.hpp>

#include <utility>

namespace demo {

void SearchScanner::scan(IsCancelled isCancelled, OnUpdate onUpdate) {
    _isCancelled = std::move(isCancelled);
    _onUpdate = std::move(onUpdate);
    _batch = {};
    _lastDelivery.restart();
    if (_isCancelled()) {
        return;
    }
    // Visit nonmatching entries too: cancellation must work even when there are no matches.
    const auto options = el::PathWalkOptions{}
                             .setTypes(el::PathType::All)
                             .setSymlinkMode(el::SymlinkMode::Use)
                             .setInfoParts(el::PathInfoPart::Type)
                             .setIgnoreErrors(true);
    const auto outcome = _root.walker().walkOrThrow(
        [this](const el::Path &path, const el::PathInfo &info) -> el::PathWalkStatus { return visit(path, info); },
        options);
    if (!_isCancelled()) {
        _batch.finished = true;
        _batch.incomplete = outcome == el::PathWalkResult::Failure;
        deliverBatch();
    }
}

auto SearchScanner::visit(const el::Path &path, const el::PathInfo &info) -> el::PathWalkStatus {
    if (_isCancelled()) {
        return el::PathWalkStatus::Stop;
    }
    if (path == _root) {
        return el::PathWalkStatus::Continue;
    }
    const auto relative = path.toRelativeOrThrow(_root);
    const auto type = info.type();
    if ((type == el::PathType::RegularFile || type == el::PathType::Directory || type == el::PathType::Symlink) &&
        _pattern->fullMatch(relative)) {
        _batch.paths.append(path);
    }
    if (_batch.paths.count() >= el::ItemCount{128} ||
        (!_batch.paths.isEmpty() && _lastDelivery.elapsed() >= el::Milliseconds{50})) {
        deliverBatch();
    }
    if (type == el::PathType::Directory && !_pattern->mayMatchDescendant(relative)) {
        return el::PathWalkStatus::Skip;
    }
    return el::PathWalkStatus::Continue;
}

void SearchScanner::deliverBatch() {
    _onUpdate(std::move(_batch));
    _batch = {};
    _lastDelivery.restart();
}

}
