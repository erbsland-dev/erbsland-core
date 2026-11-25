// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "PreCommitContext.hpp"

#include <erbsland/String.hpp>

#include <memory>
#include <optional>

namespace tool::frame {

/// Explicit participation in pre-commit, independently of command-line availability.
class PreCommitTask {
    friend class ToolApplication;

public:
    // defaults/deletions
    PreCommitTask() = default;
    virtual ~PreCommitTask() = default;
    PreCommitTask(const PreCommitTask &) = delete;
    auto operator=(const PreCommitTask &) -> PreCommitTask & = delete;

public: // identity and execution
    /// Get the stable lowercase underscore-separated task identifier.
    [[nodiscard]] virtual auto identifier() const -> el::String = 0;
    /// Get a short title for progress and diagnostics.
    [[nodiscard]] virtual auto title() const -> el::String = 0;
    /// Execute the task; operational failures are reported without stopping later tasks.
    /// @throws el::RuntimeError If a check or operation fails.
    virtual void execute() = 0;

protected: // services
    /// Access the invocation context after framework preparation.
    [[nodiscard]] auto taskContext() const -> const PreCommitContext &;

private:
    std::optional<PreCommitContext> _taskContext; ///< Current invocation services.
};

/// Shared ownership of a registered task, including dual-interface tools.
using PreCommitTaskPtr = std::shared_ptr<PreCommitTask>;

}
