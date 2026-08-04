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
    /// Describe the offset at a time point.
    /// @notest{Value type returned by Info.}
    struct OffsetDetails final {
        Seconds total{};
        Seconds dst{};
        AbbreviationOffset abbreviation{};
    };

    /// Describe an offset transition.
    /// @notest{Internal value type used by Info.}
    struct Transition final {
        Date date{};
        Seconds utcTimePoint{};
        Seconds localTimePoint{};
        Seconds dstOffset{};
        AbbreviationOffset abbreviation{};

        /// Get the time point in a requested reference.
        /// @param reference The requested time reference.
        /// @return The corresponding time point.
        [[nodiscard]] constexpr auto timePoint(TimeReference reference) const noexcept -> Seconds {
            return reference == TimeReference::Utc ? utcTimePoint : localTimePoint;
        }
    };

public:
    /// Create a lookup object from generated time-zone data.
    /// @param ruleSets The generated daylight-saving rule sets.
    /// @param continuationLines The generated zone continuation lines.
    /// @param textIdList The generated text identifier list.
    /// @param abbreviationMapping The generated abbreviation mapping.
    Info(
        std::vector<RuleSet> ruleSets,
        std::vector<ContinuationLine> continuationLines,
        std::vector<TextId> textIdList,
        std::vector<AbbreviationOffset> abbreviationMapping) noexcept;

public:
    /// Get offset details for a time point.
    /// @param secondsSinceEpoch The time point in seconds since the epoch.
    /// @param timeReference The reference for the time point.
    /// @return The offset details at the time point.
    [[nodiscard]] auto details(Seconds secondsSinceEpoch, TimeReference timeReference) const noexcept -> OffsetDetails;
    /// Get offset details for a local time point.
    /// @param localSeconds The local time point in seconds since the epoch.
    /// @param occurrence The selected occurrence in an ambiguous local-time fold.
    /// @return The offset details at the local time point.
    [[nodiscard]] auto detailsForLocal(Seconds localSeconds, TimeOccurrenceInFold occurrence) const noexcept
        -> OffsetDetails;
    /// Look up a text identifier by abbreviation offset.
    /// @param offset The abbreviation offset.
    /// @return The corresponding text identifier.
    [[nodiscard]] auto textIdFromAbbreviationOffset(AbbreviationOffset offset) const noexcept -> TextId;

private:
    /// Store a UTC candidate and its offset details.
    /// @notest{Internal value type used by Info.}
    struct Candidate final {
        Seconds utc;
        OffsetDetails details;
    };

private:
    /// Calculate the date selected by a daylight-saving rule.
    /// @param year The selected year.
    /// @param month The selected month.
    /// @param rule The daylight-saving rule.
    /// @return The calculated date.
    [[nodiscard]] static auto dayFromRule(Year year, Month month, const Rule &rule) noexcept -> Date;
    /// Find possible UTC offsets for a local time point.
    /// @param localSeconds The local time point in seconds since the epoch.
    /// @param info The generated time-zone information.
    /// @return The candidate UTC offsets.
    [[nodiscard]] static auto candidateOffsets(Seconds localSeconds, const Info &info) noexcept -> std::vector<Seconds>;
    /// Test whether one transition precedes another.
    /// @param left The first transition.
    /// @param right The second transition.
    /// @return `true` if the first transition precedes the second.
    [[nodiscard]] static auto transitionIsBefore(const Transition &left, const Transition &right) noexcept -> bool;
    /// Find the continuation line for a time point.
    /// @param secondsSinceEpoch The time point in seconds since the epoch.
    /// @param timeReference The reference for the time point.
    /// @return The matching continuation line.
    [[nodiscard]] auto continuationLine(Seconds secondsSinceEpoch, TimeReference timeReference) const noexcept
        -> const ContinuationLine &;
    /// Build transitions for a generated rule set.
    /// @param ruleSetIndex The rule-set index.
    /// @param abbreviationListOffset The abbreviation-list offset.
    /// @param lastYear The final year to include.
    /// @param standardOffset The standard UTC offset.
    /// @return The generated transitions.
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
