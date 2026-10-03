// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "OptionParser.hpp"

#include "../Option.hpp"
#include "../OptionFlag.hpp"
#include "../OptionSet.hpp"

#include "../../i18n/DisplayText.hpp"
#include "../../text/EscapeFormat.hpp"
#include "../../text/Literals.hpp"
#include "../../text/StringFormat.hpp"

namespace erbsland::options::impl {

using namespace unit;
using namespace text::literals;

auto OptionParser::collectPositionalArgument(const text::String &value, const ArgumentIndex index) -> bool {
    _positionals.emplace_back(PositionalArgument{value, index});
    return true;
}

auto OptionParser::assignPositionals() -> bool {
    if (_positionals.empty()) {
        return true;
    }

    const auto options = positionalOptions();
    if (options.empty()) {
        return makeError(
            OptionErrorReason::SyntaxError,
            i18n::displayText("options.unexpected_argument_title"_el),
            i18n::displayFormat("options.unexpected_positional_argument_description"_el)
                .build(_positionals.front().value.toEscaped(text::EscapeFormat::Display)),
            _positionals.front().index);
    }

    auto positionalIndex = std::size_t{0};
    for (auto optionIndex = std::size_t{0}; optionIndex < options.size(); ++optionIndex) {
        if (positionalIndex >= _positionals.size()) {
            break;
        }
        const auto &option = options.at(optionIndex);
        if (_storage.hasValue(option)) {
            continue;
        }

        const auto remainingValues = ArgumentCount::fromSizeT(_positionals.size() - positionalIndex);
        const auto valueLimit = positionalValueLimit(option, options, optionIndex, remainingValues);
        auto storedCount = ArgumentCount::zero();
        while (storedCount < valueLimit && positionalIndex < _positionals.size()) {
            const auto &positional = _positionals.at(positionalIndex);
            if (!storeValue(option, positional.value, positional.index, ByteIndex::zero())) {
                return false;
            }
            ++storedCount;
            ++positionalIndex;
        }
    }

    if (positionalIndex < _positionals.size()) {
        return makeError(
            OptionErrorReason::SyntaxError,
            i18n::displayText("options.unexpected_argument_title"_el),
            i18n::displayFormat("options.excess_positional_argument_description"_el)
                .build(_positionals.at(positionalIndex).value.toEscaped(text::EscapeFormat::Display)),
            _positionals.at(positionalIndex).index);
    }
    return true;
}

auto OptionParser::positionalOptions() const -> std::vector<OptionPtr> {
    auto result = std::vector<OptionPtr>{};
    for (const auto &optionSet : _activeOptionSets) {
        for (const auto &option : optionSet->options()) {
            if (!option->isDisabled() && option->isPositionalArgument()) {
                result.emplace_back(option);
            }
        }
    }
    return result;
}

auto OptionParser::requiredPositionalsAfter(const std::vector<OptionPtr> &options, const std::size_t optionIndex) const
    -> ArgumentCount {
    auto result = ArgumentCount::zero();
    for (auto index = optionIndex + 1U; index < options.size(); ++index) {
        const auto &option = options.at(index);
        if (_storage.hasValue(option) || option->hasDefaultValue()) {
            continue;
        }
        if (option->flags().isSet(OptionFlag::Required)) {
            ++result;
        }
    }
    return result;
}

auto OptionParser::positionalValueLimit(
    const OptionPtr &option,
    const std::vector<OptionPtr> &options,
    const std::size_t optionIndex,
    const ArgumentCount remainingValues) const -> ArgumentCount {
    auto limit = option->maximum();
    if (limit > remainingValues) {
        limit = remainingValues;
    }
    if (limit.isZero() || option->flags().isSet(OptionFlag::Greedy) || option->maximum().isOne()) {
        return limit;
    }

    const auto reservedValues = requiredPositionalsAfter(options, optionIndex);
    if (remainingValues <= reservedValues) {
        return ArgumentCount::zero();
    }
    auto availableValues = remainingValues - reservedValues;
    if (availableValues > limit) {
        availableValues = limit;
    }
    return availableValues;
}

}
