// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "OptionParser.hpp"

#include "../OptionErrorContext.hpp"
#include "../OptionResult.hpp"
#include "../Options.hpp"
#include "../OptionValues.hpp"

#include "../../text/Literals.hpp"

#include <utility>

namespace erbsland::options::impl {

using namespace text::literals;

OptionParser::OptionParser(OptionsPtr options, const core::CommandLineArguments &args) :
    _options{std::move(options)},
    _args{args},
    _argumentIndex{unit::ArgumentIndex::one()},
    _moduleArgumentIndex{unit::ArgumentIndex::noIndex()} {
}

auto OptionParser::parse() -> OptionResult {
    if (_args.size() > 5'000U) {
        makeError(OptionErrorReason::SyntaxError, "Too many command line arguments"_el, {});
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

auto OptionParser::makeError(
    const OptionErrorReason reason, text::StringView description, const unit::ArgumentIndex index) -> bool {
    return makeError(
        OptionErrorContext{}.setReason(reason).setDescription(std::move(description)).setArgumentIndex(index));
}

auto OptionParser::makeError(
    const OptionErrorReason reason,
    text::StringView description,
    const unit::ArgumentIndex index,
    const OptionPtr &option) -> bool {
    return makeError(
        OptionErrorContext{}
            .setReason(reason)
            .setDescription(std::move(description))
            .setArgumentIndex(index)
            .setOption(option));
}

auto OptionParser::makeError(OptionErrorContext context) -> bool {
    if (context.moduleName().isEmpty()) {
        context.setModuleName(_moduleName);
    }
    _error = std::move(context);
    return false;
}

}
