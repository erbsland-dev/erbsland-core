// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "OptionParser.hpp"

#include "OptionDisplayModel.hpp"

#include "../Option.hpp"
#include "../OptionErrorContext.hpp"
#include "../OptionResult.hpp"
#include "../Options.hpp"
#include "../OptionType.hpp"
#include "../OptionValues.hpp"

#include "../../err/LogicFailure.hpp"
#include "../../i18n/DisplayText.hpp"
#include "../../text/EscapeFormat.hpp"
#include "../../text/Literals.hpp"
#include "../../text/StringEditor.hpp"
#include "../../text/StringFormat.hpp"

#include <utility>

namespace erbsland::options::impl {

using namespace text;
using namespace unit;
using namespace text::literals;

OptionParser::OptionParser(OptionsPtr options, core::CommandLineArguments &args) :
    _options{std::move(options)},
    _args{args},
    _argumentIndex{ArgumentIndex::one()},
    _moduleArgumentIndex{ArgumentIndex::noIndex()} {
    if (_options != nullptr) {
        _options->setExecutablePath(_args.isEmpty() ? String{} : _args.first());
    }
}

OptionParser::~OptionParser() {
    cleanupSensitiveTextNoThrow();
}

auto OptionParser::parse() -> OptionResult {
    if (_args.count() > ItemCount{5'000U}) {
        makeError(
            OptionErrorReason::SyntaxError,
            i18n::displayText("options.too_many_command_line_arguments_title"_el),
            i18n::displayText("options.too_many_command_line_arguments_description"_el),
            {});
        return finishError();
    }
    if (_options == nullptr) {
        throw err::LogicFailure{"parse() was called without an options object"};
    }

    if (!validateHelpNames()) {
        return finishError();
    }

    auto status = OptionResultStatus::Success;
    if (!_options->optionModules().empty()) {
        if (_args.count() <= ItemCount::one() &&
            !_options->parserFlags().isSet(OptionParserFlag::ErrorOnMissingModule)) {
            return finishStatus(OptionResultStatus::DisplayModuleOverview);
        }
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
        _argumentIndex = ArgumentIndex::one();
    }
    _activeOptionSets = collectActiveOptionSets();
    if (_args.count() <= ItemCount::one() &&
        !_options->parserFlags().isSet(OptionParserFlag::ErrorOnEmptyRequiredPositionals)) {
        for (const auto &option : positionalOptions()) {
            if (option->flags().isSet(OptionFlag::Required)) {
                return finishStatus(OptionResultStatus::DisplayHelp);
            }
        }
    }
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
    cleanupSensitiveText();
    auto result = OptionResult{};
    result.setStatus(OptionResultStatus::Success);
    _values->setModuleName(_moduleName);
    _values->setModule(_selectedModule);
    result.setValues(_values);
    result.setSensitiveTextLocations(_sensitiveTextLocations);
    return result;
}

auto OptionParser::finishStatus(const OptionResultStatus status) -> OptionResult {
    cleanupSensitiveText();
    auto result = OptionResult{};
    result.setStatus(status);
    result.setHelpName(_helpName);
    auto values = OptionValues::create();
    values->setModuleName(_moduleName);
    values->setModule(_selectedModule);
    result.setValues(values);
    result.setSensitiveTextLocations(_sensitiveTextLocations);
    return result;
}

auto OptionParser::finishError() -> OptionResult {
    cleanupSensitiveText();
    if (_error.has_value() && !_omitErrorArguments) {
        _error->setArguments(_args);
    }
    auto result = OptionResult{};
    result.setStatus(OptionResultStatus::Error);
    result.setErrorContext(_error);
    auto values = OptionValues::create();
    values->setModuleName(_moduleName);
    values->setModule(_selectedModule);
    result.setValues(values);
    result.setSensitiveTextLocations(_sensitiveTextLocations);
    return result;
}

auto OptionParser::acceptStorageResult(const bool success) -> bool {
    if (success) {
        return true;
    }
    makeError(_storage.errorContext().value());
    return false;
}

auto OptionParser::storeValue(
    const OptionPtr &option, const String &value, const ArgumentIndex argumentIndex, const ByteIndex startIndex)
    -> bool {
    if (option != nullptr && option->type() == OptionType::Boolean) {
        auto boolean = false;
        if (!parseBooleanLiteral(value, boolean)) {
            return makeError(
                OptionErrorReason::UnexpectedValueType,
                i18n::displayText("options.invalid_boolean_value_title"_el),
                i18n::displayFormat("options.invalid_boolean_description"_el)
                    .build(value.toEscaped(EscapeFormat::Display), OptionDisplayModel::optionTitle(option)),
                argumentIndex,
                option);
        }
        return acceptStorageResult(_storage.storeBooleanValue(option, boolean, argumentIndex));
    }
    if (option != nullptr && option->type() == OptionType::SensitiveText) {
        if (!argumentIndex.isNoIndex()) {
            const auto elementIndex = ItemIndex::fromSizeT(argumentIndex.toSizeT());
            if (elementIndex.toSizeT() < _args.count().toSizeT()) {
                auto source = _args.get(elementIndex);
                source.markAsSensitive();
                _args.set(elementIndex, std::move(source));
            }
        }
        _sensitiveTextLocations.emplace_back(argumentIndex, startIndex);
    }
    return acceptStorageResult(_storage.storeValue(option, value, argumentIndex));
}

void OptionParser::cleanupSensitiveText() {
    while (_sensitiveTextCleanupIndex < _sensitiveTextLocations.size()) {
        const auto &location = _sensitiveTextLocations.at(_sensitiveTextCleanupIndex);
        ++_sensitiveTextCleanupIndex;
        if (location.argumentIndex().isNoIndex() || location.startIndex().isNoIndex()) {
            continue;
        }
        const auto argumentIndex = ItemIndex::fromSizeT(location.argumentIndex().toSizeT());
        if (argumentIndex.toSizeT() >= _args.count().toSizeT()) {
            continue;
        }
        const auto original = _args.get(argumentIndex);
        if (location.startIndex().toSizeT() > original.length().toSizeT()) {
            continue;
        }
        auto replacement = StringEditor{original.slice(ByteRange{ByteIndex::zero(), location.startIndex()})};
        replacement.append("*****"_el);
        _args.set(argumentIndex, replacement);
    }
}

void OptionParser::cleanupSensitiveTextNoThrow() noexcept {
    cleanupSensitiveText();
}

auto OptionParser::makeError(const OptionErrorReason reason, String description, const ArgumentIndex index) -> bool {
    return makeError(OptionErrorContext{}.setReason(reason).setTitle(std::move(description)).setArgumentIndex(index));
}

auto OptionParser::makeError(
    const OptionErrorReason reason, String title, String description, const ArgumentIndex index) -> bool {
    return makeError(
        OptionErrorContext{}
            .setReason(reason)
            .setTitle(std::move(title))
            .setDescription(std::move(description))
            .setArgumentIndex(index));
}

auto OptionParser::makeError(
    const OptionErrorReason reason, String description, const ArgumentIndex index, const OptionPtr &option) -> bool {
    return makeError(
        OptionErrorContext{}
            .setReason(reason)
            .setTitle(std::move(description))
            .setArgumentIndex(index)
            .setOption(option));
}

auto OptionParser::makeError(
    const OptionErrorReason reason,
    String title,
    String description,
    const ArgumentIndex index,
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
    if (context.description().isEmpty()) {
        switch (context.reason()) {
        case OptionErrorReason::SyntaxError:
            context.setDescription(i18n::displayText("options.syntax_error_description"_el));
            break;
        case OptionErrorReason::UnknownName:
            context.setDescription(i18n::displayText("options.unknown_name_description"_el));
            break;
        case OptionErrorReason::UnexpectedValueType:
            context.setDescription(i18n::displayText("options.unexpected_value_type_description"_el));
            break;
        case OptionErrorReason::ValidationError:
            context.setDescription(i18n::displayText("options.validation_error_description"_el));
            break;
        case OptionErrorReason::NotImplemented:
            context.setDescription(i18n::displayText("options.not_implemented_description"_el));
            break;
        case OptionErrorReason::None:
            context.setDescription(i18n::displayText("options.generic_error_description"_el));
            break;
        }
    }
    if (context.argumentIndex().isNoIndex()) {
        if (const auto option = context.option(); option != nullptr) {
            if (option->isPositionalArgument()) {
                context.setTitle(i18n::displayText("options.required_argument_is_missing_title"_el));
            }
        }
    }
    _error = std::move(context);
    return false;
}

}
