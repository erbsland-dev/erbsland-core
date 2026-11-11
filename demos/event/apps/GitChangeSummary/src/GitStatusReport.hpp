// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ChangeCounts.hpp"

#include <erbsland/String.hpp>
#include <erbsland/StringEditor.hpp>
#include <erbsland/StringMap.hpp>

namespace demo {

/// Summarize a stream of Git porcelain-v1 records without depending on callback boundaries.
class GitStatusReport final {
public:
    /// Directory names mapped to their change counts, ordered by name.
    using Directories = el::StringMap<ChangeCounts>;

public: // input
    /// Consume stdout bytes, preserving incomplete NUL-delimited records.
    /// @param chunk The next output block; it may end anywhere within a record.
    /// @throws ApplicationError If a complete record has invalid status syntax.
    void append(const el::String &chunk);
    /// Verify that the stream ended between complete entries.
    /// @throws ApplicationError If a record or rename source path is missing its terminator.
    void finish() const;

public: // accessors
    /// Access the accumulated directory counts.
    [[nodiscard]] auto directories() const noexcept -> const Directories & { return _directories; }

private:
    /// Consume one complete record, including the extra path of a rename or copy.
    void consumeRecord(const el::String &record);
    /// Select the first path component or the root-file label.
    static auto directoryOf(const el::String &path) -> el::String;
    /// Test whether the two status characters describe an unmerged entry.
    static auto isConflict(const el::String &status) -> bool;

private:
    Directories _directories;  ///< Counts updated only by the receiving loop.
    el::StringEditor _pending; ///< Bytes of a record split across callbacks.
    bool _needsOriginalPath{}; ///< The next record belongs to the preceding rename or copy.
};

}
