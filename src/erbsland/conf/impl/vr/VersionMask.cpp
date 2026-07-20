// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "VersionMask.hpp"

#include "../../../text/StringEditor.hpp"

#include <ranges>

namespace erbsland::conf::impl {

using namespace text::literals;

auto VersionMask::empty() noexcept -> VersionMask {
    return VersionMask{std::vector<ConfVersionRange>{}};
}

auto VersionMask::fromRanges(const std::initializer_list<ConfVersionRange> values) noexcept -> VersionMask {
    return VersionMask{normalize(std::vector<ConfVersionRange>{values})};
}

auto VersionMask::fromIntegers(const std::vector<Integer> &values) noexcept -> VersionMask {
    return fromRanges(std::vector<ConfVersionRange>{values.begin(), values.end()});
}

auto VersionMask::fromIntegers(const std::initializer_list<Integer> values) noexcept -> VersionMask {
    return fromRanges(std::vector<ConfVersionRange>{values.begin(), values.end()});
}

auto VersionMask::unionWith(const VersionMask &other) const noexcept -> VersionMask {
    // Fast-path: if either matches all, the union is all
    if (isAny() || other.isAny()) {
        return {};
    }
    if (isEmpty()) {
        return other;
    }
    if (other.isEmpty()) {
        return *this;
    }
    // Collect all ranges and normalize at once
    std::vector<ConfVersionRange> all;
    all.reserve(_ranges.size() + other._ranges.size());
    all.insert(all.end(), _ranges.begin(), _ranges.end());
    all.insert(all.end(), other._ranges.begin(), other._ranges.end());
    return VersionMask{normalize(std::move(all))};
}

auto VersionMask::intersectionWith(const VersionMask &other) const noexcept -> VersionMask {
    // Fast-paths: empty and any handling.
    if (isEmpty() || other.isEmpty()) {
        return empty();
    }
    if (isAny()) {
        return other;
    }
    if (other.isAny()) {
        return *this;
    }

    // Normalize both
    auto a = normalize(_ranges);
    auto b = normalize(other._ranges);

    // Two-pointer sweep to compute intersections
    auto result = empty();
    std::size_t i = 0;
    std::size_t j = 0;
    while (i < a.size() && j < b.size()) {
        const auto &ra = a[i];
        const auto &rb = b[j];
        const auto start = std::max(ra.first, rb.first);
        const auto end = std::min(ra.last, rb.last);
        if (start <= end) {
            // Overlap -> add intersection
            if (!result._ranges.empty() && start <= result._ranges.back().last + 1) {
                // coalesce with previous if overlapping/adjacent
                result._ranges.back().last = std::max(result._ranges.back().last, end);
            } else {
                result._ranges.emplace_back(start, end);
            }
        }
        if (ra.last < rb.last) {
            ++i;
        } else {
            ++j;
        }
    }

    return result;
}

auto VersionMask::complement() const noexcept -> VersionMask {
    // Fast paths
    if (isAny()) {
        return empty();
    }
    if (isEmpty()) {
        return VersionMask{std::vector<ConfVersionRange>{ConfVersionRange::all()}};
    }

    // Work directly on the internal (already normalized) ranges
    std::vector<ConfVersionRange> gaps;
    gaps.reserve(_ranges.size() + 1);

    const auto maxI = maxInt();

    // Gap before the first range
    const auto firstStart = _ranges.front().first;
    if (firstStart > 0) {
        gaps.emplace_back(0, firstStart - 1);
    }

    // Gaps between ranges
    for (size_t i = 1; i < _ranges.size(); ++i) {
        const auto &prev = _ranges[i - 1];
        const auto &cur = _ranges[i];
        // If prev ends at max, there is no further universe to cover
        if (prev.last == maxI) {
            break;
        }
        const auto gapStart = prev.last + 1; // safe, prev.last < maxI ensured above
        const auto gapEnd = cur.first - 1;   // cur.first >= prev.last + 2 due to normalization
        if (gapStart <= gapEnd) {
            gaps.emplace_back(gapStart, gapEnd);
        }
    }

    // Gap after the last range
    const auto &last = _ranges.back();
    if (last.last < maxI) {
        gaps.emplace_back(last.last + 1, maxI);
    }

    if (gaps.empty()) {
        return empty();
    }
    return VersionMask{std::move(gaps)};
}

auto VersionMask::isAny() const noexcept -> bool {
    return _ranges.size() == 1 && _ranges.front().first == 0 && _ranges.front().last == maxInt();
}

auto VersionMask::matches(const Integer version) const noexcept -> bool {
    return std::ranges::any_of(_ranges, [version](const auto &range) { return range.matches(version); });
}

auto VersionMask::toText() const noexcept -> text::String {
    if (_ranges.empty()) {
        return "none"_el;
    }
    text::StringEditor result;
    for (const auto &range : _ranges) {
        if (!result.isEmpty()) {
            result.append(", "_el);
        }
        const bool fromZero = (range.first == 0);
        const bool toMax = (range.last == maxInt());
        // Order of checks matters: handle full range and singleton first.
        if (fromZero && toMax) {
            // Universe
            result.append("any"_el);
        } else if (range.first == range.last) {
            // Singleton value (also covers 0-0 -> "0")
            result.append(text::StringEditor::fromInteger(range.first));
        } else if (fromZero) {
            // Up-to form
            result.append("<="_el);
            result.append(text::StringEditor::fromInteger(range.last));
        } else if (toMax) {
            // From form
            result.append(">="_el);
            result.append(text::StringEditor::fromInteger(range.first));
        } else {
            // General interval
            result.append(text::StringEditor::fromInteger(range.first));
            result.append("-"_el);
            result.append(text::StringEditor::fromInteger(range.last));
        }
    }
    return result;
}

auto VersionMask::normalize(std::vector<ConfVersionRange> ranges) noexcept -> std::vector<ConfVersionRange> {
    if (ranges.empty()) {
        return ranges;
    }
    std::ranges::sort(ranges, lessStartThenEnd);
    std::vector<ConfVersionRange> out;
    out.reserve(ranges.size());
    auto cur = ranges.front();
    for (size_t i = 1; i < ranges.size(); ++i) {
        const auto &range = ranges[i];
        const bool overlap = range.first <= cur.last;
        const bool adjacent = (cur.last < maxInt()) && (range.first == cur.last + 1);
        if (overlap || adjacent) {
            cur.last = std::max(cur.last, range.last);
        } else {
            out.push_back(cur);
            cur = range;
        }
    }
    out.push_back(cur);
    return out;
}

}
