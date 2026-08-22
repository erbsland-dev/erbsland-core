// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "HttpStaticResourceHandler.hpp"

#include "HttpStaticResourceContent.hpp"

#include "../../../../err/LogicError.hpp"
#include "../../../../resource/ResourceInfo.hpp"
#include "../../../../resource/Resources.hpp"
#include "../../../../text/Literals.hpp"
#include "../../../../text/StringEditor.hpp"

#include <utility>

namespace erbsland::network::impl {

using namespace text::literals;

HttpStaticResourceHandler::HttpStaticResourceHandler(
    resource::ResourcesConstPtr resources, text::String identifier, text::String urlPrefix) :
    network::HttpStaticResourceHandler{std::move(resources), std::move(identifier), std::move(urlPrefix)} {
}

HttpStaticResourceHandler::HttpStaticResourceHandler(
    const resource::Resources &resources, text::String identifier, text::String urlPrefix) :
    network::HttpStaticResourceHandler{resources, std::move(identifier), std::move(urlPrefix)} {
}

auto HttpStaticResourceHandler::hasPath(const path::Path &relativePath) const -> bool {
    if (relativePath.isEmpty() || !relativePath.isValid() || !relativePath.isRelative()) {
        return false;
    }
    const auto info = resources().getInfo(identifier(), portablePath(relativePath));
    return info.has_value() && !info->originalSize().isInfinite() && info->originalSize() <= maximumContentLength();
}

auto HttpStaticResourceHandler::getContent(const path::Path &relativePath) const -> HttpStaticContentPtr {
    const auto resourcePath = portablePath(relativePath);
    const auto info = resources().getInfo(identifier(), resourcePath);
    if (!info.has_value() || info->originalSize().isInfinite() || info->originalSize() > maximumContentLength()) {
        throw err::LogicError{"A probed static resource is no longer available or eligible."_el};
    }
    return std::make_shared<HttpStaticResourceContent>(
        retainedResources(), resources(), identifier(), resourcePath, info->originalSize());
}

auto HttpStaticResourceHandler::portablePath(const path::Path &relativePath) -> text::String {
    auto result = text::StringEditor{};
    for (const auto &element : relativePath.elements()) {
        if (!result.isEmpty()) {
            result.append(U'/');
        }
        result.append(element);
    }
    return text::String{result};
}

}
