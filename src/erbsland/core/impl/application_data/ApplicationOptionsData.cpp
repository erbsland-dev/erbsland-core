// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ApplicationOptionsData.hpp"

#include "../../../options/OptionManager.hpp"

#include <exception>
#include <string>
#include <utility>

namespace erbsland::core::impl {

template <typename Char>
void ApplicationOptionsData::maskNativeArguments(
    const int argumentCount, Char **arguments, const options::OptionSensitiveTextLocations &locations) noexcept {
    if (argumentCount <= 0 || arguments == nullptr) {
        return;
    }
    for (const auto &location : locations) {
        if (location.argumentIndex().isNoIndex() || location.startIndex().isNoIndex()) {
            continue;
        }
        const auto argumentIndex = location.argumentIndex().toSizeT();
        if (argumentIndex >= static_cast<std::size_t>(argumentCount) || arguments[argumentIndex] == nullptr) {
            continue;
        }
        const auto length = std::char_traits<Char>::length(arguments[argumentIndex]);
        const auto startIndex = location.startIndex().toSizeT();
        if (startIndex > length) {
            continue;
        }
        for (auto index = startIndex; index < length; ++index) {
            arguments[argumentIndex][index] = static_cast<Char>('*');
        }
    }
}

ApplicationOptionsData::ApplicationOptionsData() : _options{options::Options::create()} {
}

void ApplicationOptionsData::setCommandLineArguments(const int argc, char *argv[]) {
    if (_argumentsInitialized) {
        std::terminate();
    }
    _nativeArgumentCount = argc;
    _nativeArguments = argv;
    _nativeWideArguments = nullptr;
    _arguments = options::OptionManager::convertCommandLineArguments(argc, argv);
    _argumentsInitialized = true;
}

void ApplicationOptionsData::setCommandLineArguments(const int argc, wchar_t *argv[]) {
    if (_argumentsInitialized) {
        std::terminate();
    }
    _nativeArgumentCount = argc;
    _nativeArguments = nullptr;
    _nativeWideArguments = argv;
    _arguments = options::OptionManager::convertCommandLineArguments(argc, argv);
    _argumentsInitialized = true;
}

auto ApplicationOptionsData::commandLineArguments() const noexcept -> const CommandLineArguments & {
    return _arguments;
}

auto ApplicationOptionsData::commandLineArgumentsForParsing() noexcept -> CommandLineArguments & {
    return _arguments;
}

void ApplicationOptionsData::maskSensitiveCommandLineText(
    const options::OptionSensitiveTextLocations &locations) noexcept {
    if (_nativeArguments != nullptr) {
        maskNativeArguments(_nativeArgumentCount, _nativeArguments, locations);
    } else {
        maskNativeArguments(_nativeArgumentCount, _nativeWideArguments, locations);
    }
}

auto ApplicationOptionsData::options() noexcept -> const options::OptionsPtr & {
    return _options;
}

void ApplicationOptionsData::releaseOptions() noexcept {
    _options.reset();
}

auto ApplicationOptionsData::optionValues() noexcept -> const options::OptionValuesPtr & {
    return _optionValues;
}

void ApplicationOptionsData::setOptionValues(options::OptionValuesPtr optionValues) noexcept {
    _optionValues = std::move(optionValues);
}

}
