// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "InboxEvents.hpp"
#include "InboxWatcher_fwd.hpp"

#include <erbsland/EventSource.hpp>
#include <erbsland/EventTimer.hpp>
#include <erbsland/StringSet.hpp>

namespace demo {

/// Publishes newly visible regular files directly inside an inbox directory.
class InboxWatcher final : public el::EventSource {
    friend class InboxEvents;
    explicit InboxWatcher(el::Path directory);

public: // factories
    /// Create a one-shot watch session on the currently dispatching loop.
    /// @param directory Absolute inbox directory.
    /// @return The source to retain while monitoring.
    static auto create(el::Path directory) -> std::shared_ptr<InboxWatcher>;

public: // implement EventSource
    auto events() -> InboxEvents & override;

public: // monitoring
    /// Scan immediately, then every second until stopped. Call once on the owner loop.
    void start();
    /// End delivery, including remaining files in the current scan. Call on the owner loop.
    void stop();

private:
    void scan();

private:
    el::Path _directory;        ///< Root of the scan.
    InboxEvents _events{*this}; ///< Stable editor borrowed by callers.
    el::EventTimerPtr _timer;   ///< Retained for the session.
    el::StringSet _known;       ///< Names observed during the previous scan.
};

}
