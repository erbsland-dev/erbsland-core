// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Diagnostic.hpp"

#include "RuntimeError.hpp"

#include "../text/PlainTextRenderer.hpp"
#include "../text/TextDocument.hpp"

namespace erbsland::err {

using namespace text;

auto Diagnostic::sourceName() const noexcept -> String {
    return {};
}

auto Diagnostic::sourcePath() const noexcept -> String {
    return {};
}

auto Diagnostic::location() const noexcept -> unit::CodeLocation {
    return {};
}

auto Diagnostic::toString() const noexcept -> String {
    try {
        auto document = toTextDocument();
        return PlainTextRenderer{document}.build();
    } catch (const err::RuntimeError &) {
        return {};
    }
}

auto Diagnostic::toTextDocument() const -> TextDocument {
    return {};
}

}
