// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Diagnostic.hpp"

#include "../text/PlainTextRenderer.hpp"
#include "../text/TextDocument.hpp"

namespace erbsland::err {

auto Diagnostic::sourceName() const noexcept -> text::StringView {
    return {};
}

auto Diagnostic::sourcePath() const noexcept -> text::StringView {
    return {};
}

auto Diagnostic::location() const noexcept -> unit::CodeLocation {
    return {};
}

auto Diagnostic::toString() const noexcept -> text::StringView {
    try {
        auto document = toTextDocument();
        return text::PlainTextRenderer{document}.build();
    } catch (...) {
        return {};
    }
}

auto Diagnostic::toTextDocument(const i18n::DisplayTextMapConstPtr &) const -> text::TextDocument {
    return {};
}

auto Diagnostic::toTextDocument() const -> text::TextDocument {
    return toTextDocument({});
}

}
