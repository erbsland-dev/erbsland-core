// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "HttpStaticResourceHandler.hpp"

#include "../impl/http/static/HttpStaticResourceHandler.hpp"

#include "../../core/impl/application_data/ApplicationResourceData.hpp"
#include "../../core/impl/ApplicationDataAccess.hpp"
#include "../../err/ParameterFailure.hpp"
#include "../../resource/Resources.hpp"
#include "../../text/AsciiCategory.hpp"
#include "../../text/Literals.hpp"
#include "../../text/StringSide.hpp"

namespace erbsland::network {

using namespace err;
using namespace text;
using namespace text::literals;

HttpStaticResourceHandler::HttpStaticResourceHandler(
    resource::ResourcesConstPtr resources, String identifier, String urlPrefix) :
    HttpStaticContentHandler{std::move(urlPrefix)},
    _retainedResources{std::move(resources)},
    _identifier{std::move(identifier)} {
    if (_retainedResources == nullptr) {
        throw ParameterFailure{"A static-resource handler requires a resource provider."_el, "resources"_el};
    }
    verifyIdentifier(_identifier);
}

auto HttpStaticResourceHandler::create(resource::ResourcesConstPtr resources, String identifier, String urlPrefix)
    -> HttpStaticResourceHandlerPtr {
    return std::make_shared<impl::HttpStaticResourceHandler>(
        std::move(resources), std::move(identifier), std::move(urlPrefix));
}

auto HttpStaticResourceHandler::create(String identifier, String urlPrefix) -> HttpStaticResourceHandlerPtr {
    verifyIdentifier(identifier);
    const auto resourceData = core::impl::applicationData()->resources().get();
    return std::make_shared<impl::HttpStaticResourceHandler>(
        resource::ResourcesConstPtr{resourceData, &resourceData->resources()},
        std::move(identifier),
        std::move(urlPrefix));
}

auto HttpStaticResourceHandler::setMaximumContentLength(const unit::ByteCount value) -> HttpStaticResourceHandler & {
    if (value.isZero() || value.isInfinite()) {
        throw ParameterFailure{
            "A static-resource content limit must be positive and finite."_el, "maximumContentLength"_el};
    }
    const auto lock = lockConfiguration();
    verifyConfigurationMutable();
    _maximumContentLength = value;
    return *this;
}

auto HttpStaticResourceHandler::maximumContentLength() const noexcept -> unit::ByteCount {
    const auto lock = lockConfiguration();
    return _maximumContentLength;
}

void HttpStaticResourceHandler::verifyIdentifier(const String &identifier) {
    if (identifier.isEmpty()) {
        throw ParameterFailure{"A static-resource identifier must be a portable ASCII token."_el, "identifier"_el};
    }
    const auto [first, rest] = identifier.slice(StringSide::Front);
    if (!first.isAsciiAlphanumeric() || !rest.containsOnly(AsciiCategory::DottedName)) {
        throw ParameterFailure{"A static-resource identifier must be a portable ASCII token."_el, "identifier"_el};
    }
}

}
