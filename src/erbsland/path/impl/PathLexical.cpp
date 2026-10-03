// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PathLexical.hpp"

#include "../Path.hpp"

#include "../../text/Literals.hpp"
#include "../../text/StringList.hpp"
#include "../../unit/ItemIndex.hpp"

namespace erbsland::path::impl {

using namespace text::literals;

auto lexicalPath(const Path &path) -> Path {
    if (path.isEmpty()) {
        return {};
    }

    const auto elements = path.elements();
    if (!elements.contains("."_el) && !elements.contains(".."_el)) {
        return path; // Preserve the metadata cache when no normalization is needed.
    }
    const auto isAbsolute = path.isAbsolute();
    auto normalizedElements = text::StringList{};
    for (const auto &element : elements) {
        if (isAbsolute && element == path.root()) {
            continue;
        }
        if (element == "."_el) {
            continue;
        }
        if (element == ".."_el) {
            if (!normalizedElements.isEmpty() && normalizedElements.last() != ".."_el) {
                normalizedElements.removeLast();
            } else if (!isAbsolute) {
                normalizedElements.append(element);
            }
            continue;
        }
        normalizedElements.append(element);
    }

    if (!isAbsolute && normalizedElements.isEmpty()) {
        return Path{"."_el};
    }
    if (isAbsolute) {
        normalizedElements.insert(unit::ItemIndex::zero(), path.root());
    }
    return Path::fromElements(normalizedElements);
}

}
