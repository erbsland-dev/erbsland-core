// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "SearchUpdate.hpp"

#include <erbsland/ElapsedTimer.hpp>
#include <erbsland/PathGlobPattern.hpp>
#include <erbsland/PathInfo.hpp>
#include <erbsland/PathWalkStatus.hpp>

#include <functional>
#include <utility>

namespace demo {

/// Search one filesystem tree on a worker, delivering independent batches of matches.
class SearchScanner final {
public:
    /// A cancellation check that can also observe event-thread shutdown.
    using IsCancelled = std::function<bool()>;
    /// Receives an owned batch on the scanning thread; bind it to main before scanning.
    using OnUpdate = std::function<void(SearchUpdate)>;

public:
    /// Retain the immutable input for one search.
    /// @param root Absolute directory to search.
    /// @param pattern Immutable pattern matched relative to the root.
    SearchScanner(el::Path root, el::PathGlobPatternPtr pattern) :
        _root{std::move(root)}, _pattern{std::move(pattern)} {}

public: // scanning
    /// Walk the tree and deliver independent batches of paths.
    /// @param isCancelled Returns true when this search is no longer wanted.
    /// @param onUpdate Receives batches and one completion unless cancelled.
    void scan(IsCancelled isCancelled, OnUpdate onUpdate);

private:
    /// Match one visited entry, checking cancellation even for nonmatching paths.
    [[nodiscard]] auto visit(const el::Path &path, const el::PathInfo &info) -> el::PathWalkStatus;
    /// Transfer the current batch and restart its delivery interval.
    void deliverBatch();

private:
    el::Path _root;                  ///< Fixed absolute search root.
    el::PathGlobPatternPtr _pattern; ///< Immutable compiled query.
    IsCancelled _isCancelled;        ///< Observes superseded queries and thread shutdown.
    OnUpdate _onUpdate;              ///< Bound delivery back to main.
    SearchUpdate _batch;             ///< Matches collected since the previous delivery.
    el::ElapsedTimer _lastDelivery;  ///< Bounds the delay before a partial batch is sent.
};

}
