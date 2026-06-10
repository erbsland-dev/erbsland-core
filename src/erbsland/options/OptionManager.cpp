// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "OptionManager.hpp"

#include "OptionRenderer.hpp"
#include "Options.hpp"
#include "OptionValues.hpp"
#include "StandardOptionRenderer.hpp"

#include "impl/OptionParser.hpp"

#include "../err/OptionError.hpp"
#include "../text/StringConverter.hpp"

#include <utility>

namespace erbsland::options {

OptionManager::OptionManager() : OptionManager{Options::create()} {
}

OptionManager::OptionManager(OptionsPtr options) : _options{std::move(options)} {
    if (_options == nullptr) {
        _options = Options::create();
    }
    _renderer = StandardOptionRenderer::create();
}

void OptionManager::setRenderer(OptionRendererPtr renderer) noexcept {
    _renderer = std::move(renderer);
}

auto OptionManager::parse(const int argc, char *argv[]) -> OptionResult {
    return parse(convertCommandLineArguments(argc, argv));
}

auto OptionManager::parse(const int argc, wchar_t *argv[]) -> OptionResult {
    return parse(convertCommandLineArguments(argc, argv));
}

auto OptionManager::parse(const core::CommandLineArguments &args) -> OptionResult {
    return impl::OptionParser{_options, args}.parse();
}

auto OptionManager::parseOrThrow(const int argc, char *argv[]) -> OptionValuesPtr {
    return parseOrThrow(convertCommandLineArguments(argc, argv));
}

auto OptionManager::parseOrThrow(const int argc, wchar_t *argv[]) -> OptionValuesPtr {
    return parseOrThrow(convertCommandLineArguments(argc, argv));
}

auto OptionManager::parseOrThrow(const core::CommandLineArguments &args) -> OptionValuesPtr {
    auto result = parse(args);
    const auto &values = result.values();
    if (result.status() == OptionResultStatus::Success) {
        return values;
    }
    if (result.status() == OptionResultStatus::DisplayHelp) {
        displayHelp(values->moduleName());
        return {};
    }
    if (result.status() == OptionResultStatus::DisplayVersion) {
        displayVersion(values->moduleName());
        return {};
    }
    if (_renderer != nullptr && result.errorContext().has_value()) {
        _renderer->displayError(_options, result.errorContext().value());
    }
    if (!result.errorContext().has_value()) {
        throw err::OptionError{}; // fallback
    }
    throw err::OptionError{result.errorContext().value()};
}

void OptionManager::displayHelp(text::StringView moduleName) const {
    if (_renderer != nullptr) {
        _renderer->displayHelp(_options, std::move(moduleName));
    }
}

void OptionManager::displayVersion(text::StringView moduleName) const {
    if (_renderer != nullptr) {
        _renderer->displayVersion(_options, std::move(moduleName));
    }
}

void OptionManager::displayError(const OptionErrorContext &errorContext) const {
    if (_renderer != nullptr) {
        _renderer->displayError(_options, errorContext);
    }
}

auto OptionManager::convertCommandLineArguments(const int argc, char *argv[]) -> core::CommandLineArguments {
    auto result = core::CommandLineArguments{};
    if (argc <= 0 || argv == nullptr) {
        return result;
    }
    result.reserve(unit::ElementCount{static_cast<unit::ElementCount::Value>(argc)});
    for (int index = 0; index < argc; ++index) {
        if (argv[index] == nullptr) {
            result.append(text::StringView{});
        } else {
            result.append(text::String{std::string_view{argv[index]}});
        }
    }
    return result;
}

auto OptionManager::convertCommandLineArguments(const int argc, wchar_t *argv[]) -> core::CommandLineArguments {
    auto result = core::CommandLineArguments{};
    if (argc <= 0 || argv == nullptr) {
        return result;
    }
    result.reserve(unit::ElementCount{static_cast<unit::ElementCount::Value>(argc)});
    for (int index = 0; index < argc; ++index) {
        if (argv[index] == nullptr) {
            result.append(text::StringView{});
        } else {
            result.append(text::StringConverter{std::wstring_view{argv[index]}}.toString());
        }
    }
    return result;
}

}
