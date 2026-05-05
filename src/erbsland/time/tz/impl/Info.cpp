// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Info.hpp"

#include <algorithm>
#include <array>

namespace erbsland::time::tz::impl {

auto Info::dayFromRule(Year year, Month month, const Rule &rule) noexcept -> Date {
    const auto day = Day{static_cast<int8_t>(rule.day())};
    if (rule.dayRule() == DayRule::Exact) {
        return Date{year, month, day};
    }
    const auto dayOfWeek = DayOfWeek{static_cast<int8_t>(rule.dayOfWeek() - 1U)};
    auto date = Date{};
    if (rule.dayRule() == DayRule::AtOrLater) {
        date = Date{year, month, day};
        if (!date.isValid()) {
            date = Date::lastDay(year, month);
        }
        const auto delta = date.dayOfWeek().daysToNext(dayOfWeek);
        return delta.isZero() ? date : date.added(delta);
    }
    date = Date::lastDay(year, month);
    if (rule.dayRule() == DayRule::AtOrEarlier) {
        date = Date{year, month, day};
        if (!date.isValid()) {
            date = Date::lastDay(year, month);
        }
    }
    const auto delta = date.dayOfWeek().daysToPrevious(dayOfWeek);
    return delta.isZero() ? date : date.added(delta);
}

auto Info::candidateOffsets(Seconds localSeconds, const Info &info) noexcept -> std::vector<Seconds> {
    const auto probes = std::array<Seconds, 11>{
        Seconds{0},
        Seconds{3600},
        Seconds{-3600},
        Seconds{7200},
        Seconds{-7200},
        Seconds{10800},
        Seconds{-10800},
        Seconds{43200},
        Seconds{-43200},
        Seconds{86400},
        Seconds{-86400},
    };
    auto result = std::vector<Seconds>{};
    for (const auto probe : probes) {
        const auto offset = info.details(localSeconds + probe, TimeReference::Utc).total;
        if (std::ranges::find(result, offset) == result.end()) {
            result.emplace_back(offset);
        }
    }
    return result;
}

auto Info::transitionIsBefore(const Transition &left, const Transition &right) noexcept -> bool {
    if (left.date != right.date) {
        return left.date < right.date;
    }
    return (left.utcTimePoint + left.localTimePoint) < (right.utcTimePoint + right.localTimePoint);
}

Info::Info(
    std::vector<RuleSet> ruleSets,
    std::vector<ContinuationLine> continuationLines,
    std::vector<TextId> textIdList,
    std::vector<AbbreviationOffset> abbreviationMapping) noexcept :
    _ruleSets{std::move(ruleSets)},
    _continuationLines{std::move(continuationLines)},
    _textIdList{std::move(textIdList)},
    _abbreviationMapping{std::move(abbreviationMapping)} {
}

auto Info::details(const Seconds secondsSinceEpoch, const TimeReference timeReference) const noexcept -> OffsetDetails {
    const auto &line = continuationLine(secondsSinceEpoch, timeReference);
    if (!line.isRuleSetBased()) {
        const auto fixedOffset = line.fixedOffset();
        return {Seconds{line.standardOffset} + fixedOffset, fixedOffset, line.abbreviationOffset};
    }
    const auto date = Date::fromDaysSinceEpoch(secondsSinceEpoch.converted<Days>());
    const auto dateParts = date.parts();
    auto year = dateParts.year;
    if (dateParts.month == Month{12} && dateParts.day == Day{31}) {
        year = year.incremented();
    }
    auto transitionList =
        transitions(line.ruleSetIndex(), line.abbreviationListOffset(), year, Seconds{line.standardOffset});
    if (transitionList.empty()) {
        auto abbreviation = line.abbreviationOffset;
        if (abbreviation == 0 && line.abbreviationListOffset() > 0U && line.ruleSetIndex() < _ruleSets.size()) {
            const auto &ruleSet = _ruleSets[line.ruleSetIndex()];
            for (auto i = std::size_t{0}; i < ruleSet.rules.size(); ++i) {
                if (ruleSet.rules[i].save() != 0) {
                    continue;
                }
                const auto index = static_cast<std::size_t>(line.abbreviationListOffset() - 1U) + i;
                if (index < _abbreviationMapping.size()) {
                    abbreviation = _abbreviationMapping[index];
                    break;
                }
            }
        }
        return {Seconds{line.standardOffset}, Seconds{}, abbreviation};
    }
    auto dstOffset = Seconds{};
    auto abbreviation = AbbreviationOffset{};
    for (auto i = std::size_t{0}; i < transitionList.size(); ++i) {
        const auto current = transitionList[i].timePoint(timeReference);
        const auto hasNext = i + 1U < transitionList.size();
        const auto next = hasNext ? transitionList[i + 1U].timePoint(timeReference) : Seconds{};
        if (current <= secondsSinceEpoch && (!hasNext || next > secondsSinceEpoch)) {
            dstOffset = transitionList[i].dstOffset;
            abbreviation = transitionList[i].abbreviation;
            break;
        }
    }
    if (abbreviation == 0) {
        abbreviation = line.abbreviationOffset;
    }
    return {Seconds{line.standardOffset} + dstOffset, dstOffset, abbreviation};
}

auto Info::detailsForLocal(Seconds localSeconds, TimeOccurrenceInFold occurrence) const noexcept -> OffsetDetails {
    auto candidates = std::vector<Candidate>{};
    for (const auto offset : candidateOffsets(localSeconds, *this)) {
        const auto utc = localSeconds - offset;
        const auto detailsAtUtc = details(utc, TimeReference::Utc);
        if (utc + detailsAtUtc.total == localSeconds) {
            const auto exists = std::ranges::find(candidates, utc, &Candidate::utc) != candidates.end();
            if (!exists) {
                candidates.emplace_back(utc, detailsAtUtc);
            }
        }
    }
    std::ranges::sort(candidates, {}, &Candidate::utc);
    if (candidates.empty()) {
        return details(localSeconds, TimeReference::Local);
    }
    if (candidates.size() == 1U || occurrence == TimeOccurrenceInFold::First) {
        return candidates.front().details;
    }
    return candidates.back().details;
}

auto Info::textIdFromAbbreviationOffset(AbbreviationOffset offset) const noexcept -> TextId {
    if (offset == 0 || offset > _textIdList.size()) {
        return {};
    }
    return _textIdList[static_cast<std::size_t>(offset) - 1U];
}

auto Info::continuationLine(Seconds secondsSinceEpoch, TimeReference timeReference) const noexcept
    -> const ContinuationLine & {
    if (_continuationLines.size() == 1U) {
        return _continuationLines.front();
    }
    auto transitionPoint = Seconds{static_cast<int64_t>(_continuationLines.front().until)};
    if (timeReference == TimeReference::Local) {
        transitionPoint += Seconds{_continuationLines.front().untilOffset};
    }
    if (secondsSinceEpoch < transitionPoint) {
        return _continuationLines.front();
    }
    for (auto i = _continuationLines.size() - 1U; i > 1U; --i) {
        transitionPoint = Seconds{static_cast<int64_t>(_continuationLines[i - 1U].until)};
        if (timeReference == TimeReference::Local) {
            transitionPoint += Seconds{_continuationLines[i - 1U].untilOffset};
        }
        if (transitionPoint <= secondsSinceEpoch) {
            return _continuationLines[i];
        }
    }
    return _continuationLines[1];
}

auto Info::transitions(
    std::size_t ruleSetIndex,
    AbbreviationListOffset abbreviationListOffset,
    Year lastYear,
    Seconds standardOffset) const noexcept -> std::vector<Transition> {
    auto result = std::vector<Transition>{};
    result.reserve(8);
    if (ruleSetIndex >= _ruleSets.size()) {
        return result;
    }
    const auto &ruleSet = _ruleSets[ruleSetIndex];
    auto currentYear = lastYear;
    for (
        auto yearCounter = 0; yearCounter < 5 && result.size() < 16U && !currentYear.isFirst();
        ++yearCounter, currentYear = currentYear.decremented()) {
        for (auto i = std::size_t{0}; i < ruleSet.rules.size(); ++i) {
            const auto &rule = ruleSet.rules[i];
            if (currentYear < Year{rule.firstYear()}) {
                break;
            }
            if (currentYear > Year{rule.lastYear()}) {
                continue;
            }
            const auto transitionYear = currentYear;
            const auto month = Month{static_cast<int8_t>(rule.month() + 1U)};
            const auto date = dayFromRule(transitionYear, month, rule);
            const auto dstOffset = Minutes{rule.save()}.converted<Seconds>();
            auto abbreviation = AbbreviationOffset{};
            if (abbreviationListOffset > 0U) {
                const auto index = static_cast<std::size_t>(abbreviationListOffset - 1U) + i;
                if (index < _abbreviationMapping.size()) {
                    abbreviation = _abbreviationMapping[index];
                }
            }
            auto utcTimePoint = Seconds{};
            auto localTimePoint = Seconds{};
            const auto timePoint = date.toDaysSinceEpoch().converted<Seconds>() +
                Minutes{static_cast<int64_t>(rule.atTime())}.converted<Seconds>();
            if (rule.atTimeZone() == RuleAtTimeZone::Utc) {
                utcTimePoint = timePoint;
            } else {
                localTimePoint = timePoint;
                if (rule.atTimeZone() == RuleAtTimeZone::StandardTime) {
                    utcTimePoint = localTimePoint - standardOffset;
                }
            }
            result.emplace_back(date, utcTimePoint, localTimePoint, dstOffset, abbreviation);
        }
    }
    if (result.empty()) {
        return {};
    }
    if (result.size() == 1U && (result.front().utcTimePoint.isZero() || result.front().localTimePoint.isZero())) {
        return {};
    }
    std::ranges::stable_sort(result, transitionIsBefore);
    auto currentOffset = standardOffset;
    for (auto &transition : result) {
        if (transition.localTimePoint.isZero()) {
            transition.localTimePoint = transition.utcTimePoint + currentOffset;
        } else if (transition.utcTimePoint.isZero()) {
            transition.utcTimePoint = transition.localTimePoint - currentOffset;
        }
        currentOffset = standardOffset + transition.dstOffset;
    }
    result.erase(result.begin());
    return result;
}

}
