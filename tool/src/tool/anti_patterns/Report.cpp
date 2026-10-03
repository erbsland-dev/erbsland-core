// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Report.hpp"

#include <erbsland/CpCount.hpp>
#include <erbsland/StringEditor.hpp>
#include <erbsland/StringFormat.hpp>
#include <erbsland/StringJoins.hpp>
#include <erbsland/StringMap.hpp>
#include <erbsland/StringSet.hpp>
#include <erbsland/StringTransformations.hpp>
#include <erbsland/TruncateMode.hpp>

#include <algorithm>
#include <compare>
#include <tuple>

namespace tool::anti_patterns {

using namespace el::text::literals;

auto Report::activeCount(const el::List<Finding> &findings) const -> el::ItemCount {
    return findings.countIf([](const Finding &finding) -> bool { return !finding.isSuppressed(); });
}

auto Report::compactReason(const el::String &reason) const -> el::String {
    return el::toAsciiWhitespaceSimplified(reason).truncated(el::CpCount{80}, el::TruncateMode::End, "…"_el);
}

auto Report::collectRows(const el::List<Finding> &findings, const bool showSuppressed) -> el::List<Row> {
    auto rows = el::List<Row>{};
    auto groups = el::StringMap<Row>{};
    for (const auto &finding : findings) {
        if (finding.isSuppressed() && !showSuppressed) {
            continue;
        }
        if (finding.rule != "missing_api_documentation"_el && finding.rule != "missing_default_group_comment"_el) {
            rows.append(Row{&finding, el::ItemCount{1}});
            continue;
        }
        auto source = "active"_el;
        auto reason = el::String{};
        if (!finding.inlineReason.isEmpty()) {
            source = "inline"_el;
            reason = finding.inlineReason;
        } else if (!finding.configReason.isEmpty()) {
            source = "configuration"_el;
            reason = finding.configReason;
        }
        const auto key = el::join(finding.path.toString(), "\n"_el, source, "\n"_el, reason);
        auto row = groups.get(key, Row{});
        if (row.finding == nullptr ||
            std::tie(finding.lines, finding.range, finding.snippet) <
                std::tie(row.finding->lines, row.finding->range, row.finding->snippet)) {
            row.finding = &finding;
        }
        ++row.count;
        groups.set(key, row);
    }
    for (const auto &[key, row] : groups) {
        rows.append(row);
    }
    rows.sort([](const Row &left, const Row &right) -> bool {
        if (left.finding->severity != right.finding->severity) {
            return left.finding->severity < right.finding->severity;
        }
        if (left.count != right.count) {
            return left.count > right.count;
        }
        if (left.finding->rule != right.finding->rule) {
            return left.finding->rule < right.finding->rule;
        }
        const auto paths =
            left.finding->path.toString().compare(right.finding->path.toString(), el::Char::compareCaseFolded);
        if (paths != std::strong_ordering::equal) {
            return paths == std::strong_ordering::less;
        }
        return left.finding->lines.beginIndex() < right.finding->lines.beginIndex();
    });
    return rows;
}

auto Report::renderRow(const Row &row) const -> el::String {
    auto output = el::StringEditor{};
    const auto &finding = *row.finding;
    auto severity = "HIGH"_el;
    if (finding.severity == Severity::Medium) {
        severity = "MEDIUM"_el;
    } else if (finding.severity == Severity::Low) {
        severity = "LOW"_el;
    }
    if (finding.isSuppressed()) {
        output.append("SUPPRESSED "_el);
    }
    output.append(
        el::StringFormat{"{} {} {}:{} | {}"_el}.build(
            severity, finding.rule, finding.path.toString(), finding.lineNumber(), finding.snippet));
    if (row.count > el::ItemCount{1}) {
        output.append(el::StringFormat{" | {} more found"_el}.build(row.count - el::ItemCount{1}));
    }
    if (finding.isSuppressed()) {
        auto reason = finding.inlineReason;
        if (reason.isEmpty()) {
            reason = finding.configReason;
        }
        output.append(el::StringFormat{" | accepted: {}"_el}.build(compactReason(reason)));
    }
    output.append("\n"_el);
    return output;
}

auto Report::render(const el::List<Finding> &findings, const el::ItemCount limit, const bool showSuppressed) const
    -> el::String {
    const auto rows = collectRows(findings, showSuppressed);
    auto output = el::StringEditor{};
    auto legend = el::StringSet{};
    const auto shown = std::min(limit, rows.count());
    for (auto index = el::ItemIndex::zero(); index.isWithin(shown); ++index) {
        const auto &row = rows.getRef(index);
        output.append(renderRow(row));
        legend.insert(row.finding->rule);
    }
    if (!legend.count().isZero()) {
        output.append("Legend:\n"_el);
        for (const auto &identifier : legend) {
            output.append(
                el::StringFormat{"{}: doc/guidelines/anti_patterns/{}.rst\n"_el}.build(identifier, identifier));
        }
    }
    const auto active = activeCount(findings);
    output.append(
        el::StringFormat{"Summary: {} active, {} suppressed, {} shown (limit {}).\n"_el}.build(
            active, findings.count() - active, shown, limit));
    return el::String{output};
}

}
