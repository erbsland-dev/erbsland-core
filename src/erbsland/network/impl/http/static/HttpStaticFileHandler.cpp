// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "HttpStaticFileHandler.hpp"

#include "HttpStaticFileContent.hpp"

#include "../../../../err/LogicError.hpp"
#include "../../../../path/PathInfo.hpp"
#include "../../../../path/PathInfoParts.hpp"
#include "../../../../text/CaseSensitivity.hpp"
#include "../../../../text/Literals.hpp"

#include <utility>

namespace erbsland::network::impl {

using namespace text;
using namespace text::literals;

HttpStaticFileHandler::HttpStaticFileHandler(path::Path rootPath, text::String urlPrefix) :
    network::HttpStaticFileHandler{std::move(rootPath), std::move(urlPrefix)} {
}

auto HttpStaticFileHandler::hasPath(const path::Path &relativePath) const -> bool {
    return resolveCandidate(relativePath).has_value();
}

auto HttpStaticFileHandler::getContent(const path::Path &relativePath) const -> HttpStaticContentPtr {
    const auto candidate = resolveCandidate(relativePath);
    if (!candidate.has_value()) {
        throw err::LogicError{"A probed static file is no longer available."_el};
    }
    return std::make_shared<HttpStaticFileContent>(candidate->path, candidate->length);
}

auto HttpStaticFileHandler::resolveCandidate(const path::Path &relativePath) const -> std::optional<Candidate> {
    if (relativePath.isEmpty() || !relativePath.isValid() || !relativePath.isRelative()) {
        return std::nullopt;
    }
    auto candidatePath = rootPath();
    auto elementIndex = unit::ItemIndex::zero();
    for (const auto &element : relativePath.elements()) {
        candidatePath /= element;
        const auto info = candidatePath.info(
            path::PathInfoParts{path::PathInfoPart::Type, path::PathInfoPart::Size, path::PathInfoPart::AccessRights});
        if (!info.exists() || info.isSymlink() || info.isReparsePoint() || info.resolvedPath().isEmpty() ||
            !isBelowRoot(info.resolvedPath()) || info.resolvedPath().name() != element) {
            return std::nullopt;
        }
        ++elementIndex;
        if (elementIndex.isWithin(relativePath.elementCount())) {
            if (!info.isDirectory()) {
                return std::nullopt;
            }
            candidatePath = info.resolvedPath();
            continue;
        }
        if (!info.isRegularFile() || !info.isReadable() || info.fileSize().isInfinite()) {
            return std::nullopt;
        }
        return Candidate{info.resolvedPath(), info.fileSize()};
    }
    return std::nullopt;
}

auto HttpStaticFileHandler::isBelowRoot(const path::Path &candidate) const noexcept -> bool {
    const auto &root = rootPath();
    if (candidate.isEmpty() || candidate.elementCount() <= root.elementCount()) {
        return false;
    }
#if defined(ERBSLAND_OS_WINDOWS)
    const auto compare = cCaseInsensitive.comparisonFn();
#else
    const auto compare = CharCompareFn{};
#endif
    for (auto index = unit::ItemIndex::zero(); index.isWithin(root.elementCount()); ++index) {
        if (candidate.element(index).compare(root.element(index), compare) != std::strong_ordering::equal) {
            return false;
        }
    }
    return true;
}

}
