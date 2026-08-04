// Copyright (c) 2024-2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Parser.hpp"

#include "impl/parser/Parser.hpp"

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
