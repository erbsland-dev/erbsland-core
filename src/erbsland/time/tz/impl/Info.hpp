// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "AbbreviationOffset.hpp"
#include "ContinuationLine.hpp"
#include "RuleSet.hpp"
#include "TextId.hpp"
#include "TimeReference.hpp"

#include "../../Date.hpp"
#include "../../TimeAmounts.hpp"
#include "../../TimeOccurrenceInFold.hpp"

#include <vector>

namespace erbsland::time::tz::impl {

/// Runtime lookup object built from generated compact zone data.
/// @notest{Internal generated-data helper.}
class Info final {
public:
    struct OffsetDetails final {
        Seconds total{};
        Seconds dst{};
        AbbreviationOffset abbreviation{};
    };

    struct Transition final {
        Date date{};
        Seconds utcTimePoint{};
        Seconds localTimePoint{};
        Seconds dstOffset{};
        AbbreviationOffset abbreviation{};

        [[nodiscard]] constexpr auto timePoint(TimeReference reference) const noexcept -> Seconds {
            return reference == TimeReference::Utc ? utcTimePoint : localTimePoint;
        }
    };

public:
    Info(
        std::vector<RuleSet> ruleSets,
        std::vector<ContinuationLine> continuationLines,
        std::vector<TextId> textIdList,
        std::vector<AbbreviationOffset> abbreviationMapping) noexcept;

public:
    [[nodiscard]] auto details(Seconds secondsSinceEpoch, TimeReference timeReference) const noexcept -> OffsetDetails;
    [[nodiscard]] auto detailsForLocal(Seconds localSeconds, TimeOccurrenceInFold occurrence) const noexcept
        -> OffsetDetails;
    [[nodiscard]] auto textIdFromAbbreviationOffset(AbbreviationOffset offset) const noexcept -> TextId;

private:
    struct Candidate final {
        Seconds utc;
        OffsetDetails details;
    };

private:
    [[nodiscard]] static auto dayFromRule(Year year, Month month, const Rule &rule) noexcept -> Date;
    [[nodiscard]] static auto candidateOffsets(Seconds localSeconds, const Info &info) noexcept -> std::vector<Seconds>;
    [[nodiscard]] static auto transitionIsBefore(const Transition &left, const Transition &right) noexcept -> bool;
    [[nodiscard]] auto continuationLine(Seconds secondsSinceEpoch, TimeReference timeReference) const noexcept
        -> const ContinuationLine &;
    [[nodiscard]] auto transitions(
        std::size_t ruleSetIndex,
        AbbreviationListOffset abbreviationListOffset,
        Year lastYear,
        Seconds standardOffset) const noexcept -> std::vector<Transition>;

private:
    std::vector<RuleSet> _ruleSets;
    std::vector<ContinuationLine> _continuationLines;
    std::vector<TextId> _textIdList;
    std::vector<AbbreviationOffset> _abbreviationMapping;
};

}
