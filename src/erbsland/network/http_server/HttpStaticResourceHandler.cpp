// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "HttpStaticResourceHandler.hpp"

#include "../impl/http/static/HttpStaticResourceHandler.hpp"

#include "../../core/Application.hpp"
#include "../../err/ParameterError.hpp"
#include "../../resource/Resources.hpp"
#include "../../text/Literals.hpp"
#include "../../text/StringCharReader.hpp"

namespace erbsland::network {

using namespace text::literals;

HttpStaticResourceHandler::HttpStaticResourceHandler(
    resource::ResourcesConstPtr resources, text::String identifier, text::String urlPrefix) :
    HttpStaticContentHandler{std::move(urlPrefix)},
    _retainedResources{std::move(resources)},
    _resources{_retainedResources.get()},
    _identifier{std::move(identifier)} {
    if (_resources == nullptr) {
        throw err::ParameterError{"A static-resource handler requires a resource provider."_el, "resources"_el};
    }
    verifyIdentifier(_identifier);
}

HttpStaticResourceHandler::HttpStaticResourceHandler(
    const resource::Resources &resources, text::String identifier, text::String urlPrefix) :
    HttpStaticContentHandler{std::move(urlPrefix)}, _resources{&resources}, _identifier{std::move(identifier)} {
    verifyIdentifier(_identifier);
}

auto HttpStaticResourceHandler::create(
    resource::ResourcesConstPtr resources, text::String identifier, text::String urlPrefix)
    -> HttpStaticResourceHandlerPtr {
    return std::make_shared<impl::HttpStaticResourceHandler>(
        std::move(resources), std::move(identifier), std::move(urlPrefix));
}

auto HttpStaticResourceHandler::create(text::String identifier, text::String urlPrefix)
    -> HttpStaticResourceHandlerPtr {
    verifyIdentifier(identifier);
    return std::make_shared<impl::HttpStaticResourceHandler>(
        core::application().resources(), std::move(identifier), std::move(urlPrefix));
}

auto HttpStaticResourceHandler::setMaximumContentLength(const unit::ByteLength value) -> HttpStaticResourceHandler & {
    if (value.isZero() || value.isInfinite()) {
        throw err::ParameterError{
            "A static-resource content limit must be positive and finite."_el, "maximumContentLength"_el};
    }
    const auto lock = lockConfiguration();
    verifyConfigurationMutable();
    _maximumContentLength = value;
    return *this;
}

auto HttpStaticResourceHandler::maximumContentLength() const noexcept -> unit::ByteLength {
    const auto lock = lockConfiguration();
    return _maximumContentLength;
}

void HttpStaticResourceHandler::verifyIdentifier(const text::String &identifier) {
    if (!identifier.isValidUtf8() || identifier.isEmpty()) {
        throw err::ParameterError{"A static-resource identifier must be a portable ASCII token."_el, "identifier"_el};
    }
    auto reader = text::StringCharReader{identifier};
    auto first = true;
    while (!reader.isAtEnd()) {
        const auto character = reader.read();
        if ((first && !character.isAsciiAlphanumeric()) ||
            (!first && !character.isAsciiAlphanumeric() && character != U'.' && character != U'-' &&
                character != U'_')) {
            throw err::ParameterError{
                "A static-resource identifier must be a portable ASCII token."_el, "identifier"_el};
        }
        first = false;
    }
}

}
