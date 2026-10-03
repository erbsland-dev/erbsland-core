// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

namespace app::regex::impl {

template <typename MatchPtr>
void Execution::consumeMatch(
    const MatchPtr &match, const bool includePositions, std::uint64_t &sink, std::uint64_t &matches) {
    if (!match) {
        sink = mixSeed(sink, std::numeric_limits<std::uint64_t>::max());
        return;
    }
    ++matches;
    const auto groupCount = match->groupCount();
    sink = mixSeed(sink, groupCount);
    if (includePositions) {
        sink = mixSeed(sink, match->begin());
        sink = mixSeed(sink, match->end());
    }
    const auto content = match->content();
    sink = mixSeed(sink, content.characterLength().toRawValue());
    sink = mixSeed(sink, content.toHash());
    for (auto index = std::size_t{}; index < groupCount; ++index) {
        const auto groupIndex = static_cast<el::re::CaptureGroupIndex>(index);
        const auto &group = match->group(groupIndex);
        if (includePositions) {
            sink = mixSeed(sink, group.begin());
            sink = mixSeed(sink, group.end());
        }
        sink = mixSeed(sink, group.name().toHash());
        const auto groupContent = match->content(groupIndex);
        sink = mixSeed(sink, groupContent.characterLength().toRawValue());
        sink = mixSeed(sink, groupContent.toHash());
    }
}

template <typename MatchType>
void Execution::consumeStandardMatch(
    const MatchType *match, const std::string &subject, std::uint64_t &sink, std::uint64_t &matches) {
    if (match == nullptr) {
        sink = mixSeed(sink, std::numeric_limits<std::uint64_t>::max());
        return;
    }
    ++matches;
    sink = mixSeed(sink, match->size());
    const auto matchBegin = static_cast<std::size_t>(std::distance(subject.cbegin(), (*match)[0].first));
    const auto matchEnd = static_cast<std::size_t>(std::distance(subject.cbegin(), (*match)[0].second));
    sink = mixSeed(sink, matchBegin);
    sink = mixSeed(sink, matchEnd);
    const auto content = el::String{std::string_view{(*match)[0].first, (*match)[0].second}};
    sink = mixSeed(sink, content.characterLength().toRawValue());
    sink = mixSeed(sink, content.toHash());
    for (auto index = std::size_t{}; index < match->size(); ++index) {
        const auto &group = (*match)[index];
        const auto groupBegin = static_cast<std::size_t>(std::distance(subject.cbegin(), group.first));
        const auto groupEnd = static_cast<std::size_t>(std::distance(subject.cbegin(), group.second));
        sink = mixSeed(sink, groupBegin);
        sink = mixSeed(sink, groupEnd);
        sink = mixSeed(sink, el::String{}.toHash());
        const auto groupContent = el::String{std::string_view{group.first, group.second}};
        sink = mixSeed(sink, groupContent.characterLength().toRawValue());
        sink = mixSeed(sink, groupContent.toHash());
    }
}

template <typename StringType>
void Execution::executeOnString(
    const StringType &subject, const el::re::RegExPtr &expression, std::uint64_t &sink, std::uint64_t &matches) const {
    const auto includePositions = !_prepared.scenario.comparisonName.isEmpty();
    switch (_prepared.scenario.useCase) {
    case UseCase::Match:
        consumeMatch(expression->match(subject), includePositions, sink, matches);
        break;
    case UseCase::FullMatch:
        consumeMatch(expression->fullMatch(subject), includePositions, sink, matches);
        break;
    case UseCase::FindFirst:
        consumeMatch(expression->findFirst(subject), includePositions, sink, matches);
        break;
    case UseCase::FindAll:
        for (const auto &match : expression->findAll(subject)) {
            consumeMatch(match, includePositions, sink, matches);
        }
        break;
    case UseCase::CollectAll:
        for (const auto &match : expression->collectAll(subject)) {
            consumeMatch(match, includePositions, sink, matches);
        }
        break;
    default:
        workloadError("Unsupported string regex workload path."_el);
    }
}

}
