// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "HttpStaticFileHandler.hpp"

#include "../impl/http/static/HttpStaticFileHandler.hpp"

#include "../../err/ParameterError.hpp"
#include "../../path/PathError.hpp"
#include "../../path/PathErrorContext.hpp"
#include "../../path/PathInfo.hpp"
#include "../../path/PathInfoParts.hpp"
#include "../../text/Literals.hpp"

namespace erbsland::network {

using namespace text::literals;

HttpStaticFileHandler::HttpStaticFileHandler(path::Path rootPath, text::String urlPrefix) :
    HttpStaticContentHandler{std::move(urlPrefix)} {
    if (rootPath.isEmpty() || !rootPath.isValid()) {
        throw err::ParameterError{"A static-file handler requires a valid filesystem root."_el, "rootPath"_el};
    }
    rootPath = rootPath.toAbsoluteOrThrow();
    const auto info = rootPath.info(path::PathInfoParts{path::PathInfoPart::Type, path::PathInfoPart::AccessRights});
    if (!info.exists() || !info.isDirectory() || !info.isReadable() || info.isSymlink() || info.isReparsePoint() ||
        info.resolvedPath().isEmpty()) {
        throw path::PathError{path::PathErrorContext{
            "Static-content root is unavailable"_el,
            "The root must be an accessible directory and must not be a symbolic link or reparse point."_el}
                .setSourcePath(rootPath.toString())};
    }
    _rootPath = info.resolvedPath();
}

auto HttpStaticFileHandler::create(path::Path rootPath, text::String urlPrefix) -> HttpStaticFileHandlerPtr {
    return std::make_shared<impl::HttpStaticFileHandler>(std::move(rootPath), std::move(urlPrefix));
}

}
