// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "HttpStaticResourceHandler.hpp"

#include "HttpStaticResourceContent.hpp"

#include "../../../../err/LogicFailure.hpp"
#include "../../../../resource/ResourceInfo.hpp"
#include "../../../../resource/Resources.hpp"
#include "../../../../text/Literals.hpp"
#include "../../../../text/StringEditor.hpp"

#include <utility>

namespace erbsland::network::impl {

using namespace text;
using namespace text::literals;

HttpStaticResourceHandler::HttpStaticResourceHandler(
    resource::ResourcesConstPtr resources, String identifier, String urlPrefix) :
    network::HttpStaticResourceHandler{std::move(resources), std::move(identifier), std::move(urlPrefix)} {
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
        throw err::LogicFailure{"A probed static resource is no longer available or eligible."_el};
    }
    return std::make_shared<HttpStaticResourceContent>(
        retainedResources(), identifier(), resourcePath, info->originalSize());
}

auto HttpStaticResourceHandler::portablePath(const path::Path &relativePath) -> String {
    auto result = StringEditor{};
    for (const auto &element : relativePath.elements()) {
        if (!result.isEmpty()) {
            result.append(U'/');
        }
        result.append(element);
    }
    return result;
}

}
