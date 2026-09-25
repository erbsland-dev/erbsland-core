// Copyright (c) 2024-2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Parser.hpp"

#include "impl/parser/Parser.hpp"
#include "impl/placeholder/ErrorAdapter.hpp"

#include "../err/LogicError.hpp"
#include "../err/ParameterError.hpp"
#include "../text/placeholder/EnvironmentSource.hpp"
#include "../text/placeholder/impl/Name.hpp"
#include "../text/placeholder/ReplacerError.hpp"
#include "../text/placeholder/TextFilter.hpp"
#include "../text/placeholder/VariableSource.hpp"

#include <exception>

namespace erbsland::conf {

using namespace text::literals;

void Parser::setSourceResolver(const SourceResolverPtr &sourceResolver) noexcept {
    _settings.sourceResolver = sourceResolver;
}

void Parser::setAccessCheck(const AccessCheckPtr &accessCheck) noexcept {
    _settings.accessCheck = accessCheck;
}

void Parser::setSignatureValidator(const SignatureValidatorPtr &signatureValidator) noexcept {
    _settings.signatureValidator = signatureValidator;
}

void Parser::addPlaceholderSource(const text::placeholder::SourcePtr &source) {
    _settings.placeholderRegistry->addSource(source);
}

void Parser::removePlaceholderSource(const text::placeholder::SourcePtr &source) noexcept {
    _settings.placeholderRegistry->removeSource(source);
}

void Parser::addPlaceholderFilter(const text::placeholder::FilterPtr &filter) {
    _settings.placeholderRegistry->addFilter(filter);
}

void Parser::removePlaceholderFilter(const text::placeholder::FilterPtr &filter) noexcept {
    _settings.placeholderRegistry->removeFilter(filter);
}

void Parser::addPlaceholderEnvironmentSource(const text::String &name) {
    addPlaceholderSource(std::make_shared<text::placeholder::EnvironmentSource>(name));
}

void Parser::setPlaceholderVariableSource(text::StringMap<text::String> variables, const text::String &name) {
    try {
        const auto effectiveName = name.isEmpty() ? "var"_el : name;
        const auto normalizedName = text::placeholder::impl::normalizeName(effectiveName);
        const auto source = _settings.placeholderRegistry->source(normalizedName);
        if (source != nullptr) {
            const auto variableSource = std::dynamic_pointer_cast<text::placeholder::VariableSource>(source);
            if (variableSource == nullptr) {
                throw err::LogicError{"The placeholder source name belongs to another provider."_el};
            }
            variableSource->setVariables(std::move(variables));
            return;
        }
        addPlaceholderSource(std::make_shared<text::placeholder::VariableSource>(std::move(variables), effectiveName));
    } catch (const text::placeholder::ReplacerError &error) {
        throw ConfError{
            impl::placeholder::toConfErrorCategory(error.category()), {}, error.reason(), std::current_exception()};
    }
}

void Parser::addPlaceholderTextFilters() {
    addPlaceholderFilter(std::make_shared<text::placeholder::TextFilter>());
}

auto Parser::parseOrThrow(const SourcePtr &source) -> DocumentPtr {
    _lastError = std::nullopt;
    if (source == nullptr) {
        throw err::ParameterError{"Source cannot be null."_el, "source"_el};
    }
    impl::Parser parserImplementation(source, _settings);
    return parserImplementation.parse();
}

auto Parser::parse(const SourcePtr &source) -> DocumentPtr {
    try {
        _lastError = std::nullopt;
        if (source == nullptr) {
            throw err::ParameterError{"Source cannot be null."_el, "source"_el};
        }
        impl::Parser parserImplementation(source, _settings);
        return parserImplementation.parse();
    } catch (const ConfError &error) {
        _lastError = error.context();
        return {};
    }
}

auto Parser::lastError() const noexcept -> ConfErrorContext {
    return _lastError.value_or(ConfErrorContext{});
}

}
