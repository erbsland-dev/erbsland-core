// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "PlatformErrorContext.hpp"

#include "../event/impl/CommonEventEditor.hpp"
#include "../text/String.hpp"
#include "../unit/ExitCode.hpp"

#include <functional>

namespace erbsland::system {

/// Editor for owner-loop subprocess handlers; empty callbacks clear handlers.
/// @tested{SubprocessEventTest}
class SubprocessEventEditor : public event::impl::CommonEventEditor {
public:
    /// A lifecycle notification.
    using EventFn = std::function<void()>;
    /// Stdout or stderr bytes, aligned at valid UTF-8 boundaries; BOM removal follows SubprocessOptions.
    using OutputFn = std::function<void(const text::String &)>;
    /// An ordinary process exit, including nonzero exit codes.
    using ExitFn = std::function<void(unit::ExitCode)>;
    /// An abnormal workflow outcome with native diagnostic details.
    using ErrorFn = std::function<void(const PlatformErrorContext &)>;

public:
    // defaults
    ~SubprocessEventEditor() override = default;

public: // handlers
    /// Set the successful launch handler.
    /// @param callback The replacement handler, or empty to clear it.
    /// @return This editor.
    virtual auto onStarted(EventFn callback) -> SubprocessEventEditor & = 0;
    /// Set the standard output handler.
    /// @param callback The replacement handler, or empty to clear it.
    /// @return This editor.
    virtual auto onStandardOutput(OutputFn callback) -> SubprocessEventEditor & = 0;
    /// Set the standard error output handler.
    /// @param callback The replacement handler, or empty to clear it.
    /// @return This editor.
    virtual auto onStandardError(OutputFn callback) -> SubprocessEventEditor & = 0;
    /// Set the once-only event-buffer overflow handler.
    /// @param callback The replacement handler, or empty to clear it.
    /// @return This editor.
    virtual auto onOutputTruncated(EventFn callback) -> SubprocessEventEditor & = 0;
    /// Set the ordinary exit handler.
    /// @param callback The replacement handler, or empty to clear it.
    /// @return This editor.
    virtual auto onExit(ExitFn callback) -> SubprocessEventEditor & = 0;
    /// Set the abnormal outcome handler.
    /// @param callback The replacement handler, or empty to clear it.
    /// @return This editor.
    virtual auto onError(ErrorFn callback) -> SubprocessEventEditor & = 0;
    /// Set the final completion handler.
    /// @param callback The replacement handler, or empty to clear it.
    /// @return This editor.
    virtual auto onFinal(EventFn callback) -> SubprocessEventEditor & = 0;

protected:
    /// Create an editor for its source and owner target.
    /// @param source The owning source.
    /// @param target The callback target.
    SubprocessEventEditor(event::EventSourcePtr source, event::EventTargetPtr target);
};

}
