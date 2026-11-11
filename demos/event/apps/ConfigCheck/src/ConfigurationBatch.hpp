// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/EventLoop.hpp>
#include <erbsland/ItemCount.hpp>
#include <erbsland/List.hpp>
#include <erbsland/Path.hpp>

namespace demo {

/// Check independent configuration files on one worker, optionally limiting failed files.
/// Schedule once before starting the worker and keep this object alive until it is joined.
/// All mutable state belongs to the worker; read the results only after joining it.
class ConfigurationBatch final {
public:
    /// Retain the files in their command-line order.
    /// @param paths The top-level configuration files to parse.
    /// @param maximumErrors Stop after this many failed files; zero checks every file.
    explicit ConfigurationBatch(el::PathList paths, el::ItemCount maximumErrors = {});

public: // results
    /// The files whose complete configuration, including includes, parsed successfully.
    [[nodiscard]] auto passedPaths() const noexcept -> const el::PathList & { return _passedPaths; }
    /// The number of top-level files attempted, including failed files.
    [[nodiscard]] auto checkedCount() const noexcept -> el::ItemCount { return _checkedCount; }

public: // scheduling
    /// Queue the checks and a final quit callback on a fresh loop configured with Store.
    /// Installs a custom handler only when an error limit was requested.
    /// @param loop The worker loop, which must outlive this batch's queued callbacks and handler.
    void schedule(el::EventLoop &loop);

private:
    void installErrorLimit(el::EventLoop &loop);
    void checkFile(const el::Path &path);

private:
    el::PathList _paths;          ///< Input files, in the requested order.
    el::ItemCount _maximumErrors; ///< Zero means no error limit.
    el::ItemCount _failureCount;  ///< Failed callbacks counted by the optional handler.
    el::ItemCount _checkedCount;  ///< Files attempted on the worker.
    el::PathList _passedPaths;    ///< Successful files, read after the worker is joined.
};

}
