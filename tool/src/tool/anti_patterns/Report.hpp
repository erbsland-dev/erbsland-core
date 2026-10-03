// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Finding.hpp"

#include <erbsland/ItemCount.hpp>
#include <erbsland/List.hpp>
#include <erbsland/String.hpp>

namespace tool::anti_patterns {

/// Build the stable, bounded report shown to agents and developers.
class Report final {
    /// One representative finding and the number of equivalent report entries.
    struct Row final {
        const Finding *finding{}; ///< Borrowed representative, valid while the input list is retained.
        el::ItemCount count{};    ///< Number of represented findings.
    };

public:
    /// Render selected findings and a complete active/suppressed summary.
    [[nodiscard]] auto render(const el::List<Finding> &findings, el::ItemCount limit, bool showSuppressed) const
        -> el::String;
    /// Count findings that lack an accepted exception.
    [[nodiscard]] auto activeCount(const el::List<Finding> &findings) const -> el::ItemCount;

private:
    /// Group and sort visible findings without altering the original findings.
    static auto collectRows(const el::List<Finding> &findings, bool showSuppressed) -> el::List<Row>;
    /// Render one representative row with suppression and grouping details.
    auto renderRow(const Row &row) const -> el::String;
    /// Bound an acceptance reason to one compact report line.
    [[nodiscard]] auto compactReason(const el::String &reason) const -> el::String;
};

}
