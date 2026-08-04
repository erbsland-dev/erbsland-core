// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "OptionParser.hpp"

#include "../Option.hpp"
#include "../OptionError.hpp"
#include "../OptionModule.hpp"
#include "../Options.hpp"
#include "../OptionSet.hpp"
#include "../OptionValues.hpp"

#include <utility>

namespace erbsland::options::impl {

auto OptionParser::runSelectedModulePreCallback() -> bool {
    if (_selectedModule == nullptr || !_selectedModule->preParsingFn()) {
        return true;
    }
    try {
        _selectedModule->preParsingFn()(_selectedModule);
    } catch (const OptionError &error) {
        return makeCallbackError(error.context(), OptionErrorReason::None);
    }
    return true;
}

auto OptionParser::runActiveOptionSetPreCallbacks() -> bool {
    for (const auto &optionSet : _activeOptionSets) {
        if (!optionSet->preParsingFn()) {
            continue;
        }
        try {
            optionSet->preParsingFn()(optionSet);
        } catch (const OptionError &error) {
            return makeCallbackError(error.context(), OptionErrorReason::None, optionSet);
        }
    }
    return true;
}

auto OptionParser::runValidators() -> bool {
    for (const auto &optionSet : _activeOptionSets) {
        for (const auto &option : optionSet->options()) {
            if (option->isDisabled() || !option->validateFn()) {
                continue;
            }
            const auto optionValue = valueForOption(option);
            if (optionValue == nullptr) {
                continue;
            }
            try {
                option->validateFn()(optionValue, _values);
            } catch (const OptionError &error) {
                return makeValidatorError(error.context(), optionSet, option);
            }
        }
    }
    return true;
}

auto OptionParser::runPostCallbacks() -> bool {
    for (const auto &optionSet : _activeOptionSets) {
        if (!optionSet->postParsingFn()) {
            continue;
        }
        try {
            optionSet->postParsingFn()(_values);
        } catch (const OptionError &error) {
            return makeCallbackError(error.context(), OptionErrorReason::ValidationError, optionSet);
        }
    }
    if (_selectedModule != nullptr && _selectedModule->postParsingFn()) {
        try {
            _selectedModule->postParsingFn()(_values);
        } catch (const OptionError &error) {
            return makeCallbackError(error.context(), OptionErrorReason::ValidationError);
        }
    }
    return true;
}

auto OptionParser::valueForOption(const OptionPtr &option) const -> OptionValuePtr {
    if (_values == nullptr) {
        return {};
    }
    for (const auto &name : option->names()) {
        const auto value = _values->value(name);
        if (value != nullptr) {
            return value;
        }
    }
    return {};
}

auto OptionParser::makeCallbackError(
    OptionErrorContext context, const OptionErrorReason defaultReason, const OptionSetPtr &optionSet) -> bool {
    if (context.reason() == OptionErrorReason::None && defaultReason != OptionErrorReason::None) {
        context.setReason(defaultReason);
    }
    if (optionSet != nullptr && context.optionSet() == nullptr) {
        context.setOptionSet(optionSet);
    }
    return makeError(std::move(context));
}

auto OptionParser::makeValidatorError(
    OptionErrorContext context, const OptionSetPtr &optionSet, const OptionPtr &option) -> bool {
    if (context.reason() == OptionErrorReason::None) {
        context.setReason(OptionErrorReason::ValidationError);
    }
    if (context.optionSet() == nullptr) {
        context.setOptionSet(optionSet);
    }
    if (context.option() == nullptr) {
        context.setOption(option);
    }
    if (context.argumentIndex().isNoIndex()) {
        if (const auto value = valueForOption(option); value != nullptr) {
            context.setArgumentIndex(value->argumentIndex());
        }
    }
    return makeError(std::move(context));
}

}
