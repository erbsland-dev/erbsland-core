// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/ByteBlock.hpp>
#include <erbsland/ItemCount.hpp>
#include <erbsland/StringEditor.hpp>

namespace demo {

/// Counts complete LF-delimited log lines, retaining only their severity prefix between reads.
class LogCounts final {
public: // input
    /// Consume another chunk. An empty chunk ends the final unterminated line.
    /// @param bytes Log bytes in input order.
    void consume(const el::ByteBlock &bytes);
    /// Print cumulative totals.
    /// @param label Report label (periodic or final).
    void report(const el::String &label) const;

private:
    void endLine();

private:
    el::StringEditor _prefix;                   ///< First six characters suffice for all supported labels.
    el::ItemCount _info, _warn, _error, _other; ///< Completed lines in each category.
};

}
