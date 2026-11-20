// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "InboxWatcher_fwd.hpp"

#include <erbsland/EventEditor.hpp>
#include <erbsland/Path.hpp>

#include <functional>

namespace demo {

/// The handler configuration owned by one watcher.
class InboxEvents final : public el::EventEditor {
    friend class InboxWatcher;
    explicit InboxEvents(InboxWatcher &watcher) : _watcher{watcher} {}

public:
    /// Called on the owner loop for one file.
    using Arrival = std::function<void(const el::Path &)>;

public: // implement EventEditor
    auto source() const -> el::EventSourcePtr override;
    auto target() const noexcept -> el::EventTargetPtr override;

public: // notifications
    /// Replace the arrival handler on the owner loop.
    /// @param handler Called for subsequent files. An empty handler clears the slot.
    /// @return This editor.
    auto onFileArrived(Arrival handler) -> InboxEvents &;

private:
    InboxWatcher &_watcher;            ///< Enclosing source outlives its editor.
    std::shared_ptr<Arrival> _arrival; ///< The active invocation can retain a replaced handler.
};

}
