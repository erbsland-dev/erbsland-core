// Copyright (c) 2024-2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Parser.hpp"

#include "impl/parser/Parser.hpp"
#include "impl/placeholder/EnvironmentPlaceholderSource.hpp"
#include "impl/placeholder/TextPlaceholderFilter.hpp"
#include "impl/placeholder/VariablePlaceholderSource.hpp"

#include "../err/LogicError.hpp"
#include "../err/ParameterError.hpp"

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

void Parser::addPlaceholderSource(const PlaceholderSourcePtr &source) {
    _settings.placeholderResolver->addSource(source);
}

void Parser::removePlaceholderSource(const PlaceholderSourcePtr &source) noexcept {
    _settings.placeholderResolver->removeSource(source);
}

void Parser::addPlaceholderFilter(const PlaceholderFilterPtr &filter) {
    _settings.placeholderResolver->addFilter(filter);
}

void Parser::removePlaceholderFilter(const PlaceholderFilterPtr &filter) noexcept {
    _settings.placeholderResolver->removeFilter(filter);
}

void Parser::enableEnvironmentPlaceholderSource() {
    addPlaceholderSource(std::make_shared<impl::placeholder::EnvironmentPlaceholderSource>());
}

void Parser::setPlaceholderVariables(text::StringMap<text::String> variables) {
    const auto source = _settings.placeholderResolver->source("var"_el);
    if (source != nullptr) {
        const auto variableSource = std::dynamic_pointer_cast<impl::placeholder::VariablePlaceholderSource>(source);
        if (variableSource == nullptr) {
            throw err::LogicError{"Placeholder source name is already registered: var"_el};
        }
        variableSource->setVariables(std::move(variables));
        return;
    }
    addPlaceholderSource(std::make_shared<impl::placeholder::VariablePlaceholderSource>(std::move(variables)));
}

void Parser::enableTextPlaceholderFilters() {
    addPlaceholderFilter(std::make_shared<impl::placeholder::TextPlaceholderFilter>());
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
