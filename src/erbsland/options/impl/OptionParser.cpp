// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "OptionParser.hpp"

#include "../Option.hpp"
#include "../OptionErrorContext.hpp"
#include "../OptionResult.hpp"
#include "../Options.hpp"
#include "../OptionValues.hpp"

#include "../../i18n/DisplayTextMap.hpp"
#include "../../text/Literals.hpp"

#include <utility>

namespace erbsland::options::impl {

using namespace text::literals;

OptionParser::OptionParser(
    OptionsPtr options, const core::CommandLineArguments &args, i18n::DisplayTextMapConstPtr displayText) :
    _options{std::move(options)},
    _args{args},
    _displayText{displayText != nullptr ? std::move(displayText) : i18n::DisplayTextMap::defaultMap()},
    _argumentIndex{unit::ArgumentIndex::one()},
    _moduleArgumentIndex{unit::ArgumentIndex::noIndex()} {
    if (_options != nullptr) {
        _options->setExecutablePath(_args.isEmpty() ? text::String{} : _args.first());
    }
}

auto OptionParser::parse() -> OptionResult {
    if (_args.count() > unit::ElementCount{5'000U}) {
        makeError(
            OptionErrorReason::SyntaxError,
            "Too many command-line arguments"_el,
            "This command accepts at most 5,000 command-line arguments."_el,
            {});
        return finishError();
    }

    auto status = OptionResultStatus::Success;
    if (_options != nullptr && !_options->optionModules().empty()) {
        if (!prepareModuleParsing()) {
            return finishError();
        }
        if (isHelpOrVersionRequest(status, _argumentIndex)) {
            return finishStatus(status);
        }
        if (!runSelectedModulePreCallback()) {
            return finishError();
        }
    } else {
        if (isHelpOrVersionRequest(status)) {
            return finishStatus(status);
        }
        _argumentIndex = unit::ArgumentIndex::one();
    }
    _activeOptionSets = collectActiveOptionSets();
    if (!runActiveOptionSetPreCallbacks()) {
        return finishError();
    }
    if (!validateOptionNames()) {
        return finishError();
    }
    if (!parseActiveOptions()) {
        return finishError();
    }
    if (!assignPositionals()) {
        return finishError();
    }
    if (!acceptStorageResult(_storage.applyDefaults(_activeOptionSets))) {
        return finishError();
    }
    if (!acceptStorageResult(_storage.checkRequiredOptions(_activeOptionSets))) {
        return finishError();
    }
    _values = _storage.values();
    if (!runValidators()) {
        return finishError();
    }
    if (!runPostCallbacks()) {
        return finishError();
    }
    return finishSuccess();
}

auto OptionParser::finishSuccess() -> OptionResult {
    auto result = OptionResult{};
    result.setStatus(OptionResultStatus::Success);
    _values->setModuleName(_moduleName);
    _values->setModule(_selectedModule);
    result.setValues(_values);
    return result;
}

auto OptionParser::finishStatus(const OptionResultStatus status) -> OptionResult {
    auto result = OptionResult{};
    result.setStatus(status);
    auto values = OptionValues::create();
    values->setModuleName(_moduleName);
    values->setModule(_selectedModule);
    result.setValues(values);
    return result;
}

auto OptionParser::finishError() -> OptionResult {
    auto result = OptionResult{};
    result.setStatus(OptionResultStatus::Error);
    result.setErrorContext(_error);
    auto values = OptionValues::create();
    values->setModuleName(_moduleName);
    values->setModule(_selectedModule);
    result.setValues(values);
    return result;
}

auto OptionParser::acceptStorageResult(const bool success) -> bool {
    if (success) {
        return true;
    }
    makeError(_storage.errorContext().value());
    return false;
}

auto OptionParser::makeError(const OptionErrorReason reason, text::String description, const unit::ArgumentIndex index)
    -> bool {
    return makeError(OptionErrorContext{}.setReason(reason).setTitle(std::move(description)).setArgumentIndex(index));
}

auto OptionParser::makeError(
    const OptionErrorReason reason, text::String title, text::String description, const unit::ArgumentIndex index)
    -> bool {
    return makeError(
        OptionErrorContext{}
            .setReason(reason)
            .setTitle(std::move(title))
            .setDescription(std::move(description))
            .setArgumentIndex(index));
}

auto OptionParser::makeError(
    const OptionErrorReason reason, text::String description, const unit::ArgumentIndex index, const OptionPtr &option)
    -> bool {
    return makeError(
        OptionErrorContext{}
            .setReason(reason)
            .setTitle(std::move(description))
            .setArgumentIndex(index)
            .setOption(option));
}

auto OptionParser::makeError(
    const OptionErrorReason reason,
    text::String title,
    text::String description,
    const unit::ArgumentIndex index,
    const OptionPtr &option) -> bool {
    return makeError(
        OptionErrorContext{}
            .setReason(reason)
            .setTitle(std::move(title))
            .setDescription(std::move(description))
            .setArgumentIndex(index)
            .setOption(option));
}

auto OptionParser::makeError(OptionErrorContext context) -> bool {
    if (context.options() == nullptr) {
        context.setOptions(_options);
    }
    if (context.module() == nullptr) {
        context.setModule(_selectedModule);
    }
    if (context.arguments().isEmpty()) {
        context.setArguments(_args);
    }
    context.setDisplayText(_displayText);
    if (context.description().isEmpty()) {
        switch (context.reason()) {
        case OptionErrorReason::SyntaxError:
            context.setDescription("Review the marked argument and the usage information below."_el);
            break;
        case OptionErrorReason::UnknownName:
            context.setDescription("The marked name is not available for this command."_el);
            break;
        case OptionErrorReason::UnexpectedValueType:
            context.setDescription("The marked value does not meet the requirements of this option."_el);
            break;
        case OptionErrorReason::ValidationError:
            context.setDescription("The supplied options did not pass validation."_el);
            break;
        case OptionErrorReason::NotImplemented:
            context.setDescription("This option operation has not been implemented."_el);
            break;
        case OptionErrorReason::None:
            context.setDescription("Option processing stopped without additional details."_el);
            break;
        }
    }
    if (context.argumentIndex().isNoIndex()) {
        if (const auto option = context.option(); option != nullptr) {
            if (option->isPositionalArgument()) {
                context.setTitle("Required argument is missing"_el);
            }
        }
    }
    _error = std::move(context);
    return false;
}

}
