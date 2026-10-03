// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "SubprocessErrorContext.hpp"

#include "../../../text/StringFormat.hpp"
#include "../../../text/TextDocument.hpp"

namespace erbsland::system::impl {

using namespace text;

SubprocessErrorContext::SubprocessErrorContext(SubprocessExitStatus status) : _status{status} {
}

SubprocessErrorContext::SubprocessErrorContext(String description) : _description{std::move(description)} {
}

auto SubprocessErrorContext::category() const noexcept -> PlatformErrorCategory {
    return PlatformErrorCategory::Unknown;
}

auto SubprocessErrorContext::toString() const noexcept -> String {
    using namespace text::literals;
    if (!_description.isEmpty()) {
        return _description;
    }
    if (_status.wasSignaled()) {
        return StringFormat{"Subprocess terminated by signal {}."_el}.build(*_status.terminationSignal());
    }
    return StringFormat{"Subprocess terminated abnormally with exit code {}."_el}.build(*_status.exitCode());
}

auto SubprocessErrorContext::toTextDocument() const -> TextDocument {
    auto document = TextDocument{};
    auto fields = document.add(TextNodeType::FieldList);
    auto item = fields->add(TextNodeType::FieldItem);
    using namespace text::literals;
    item->add(TextNodeType::FieldLabel)->addText("subprocess"_el);
    item->add(TextNodeType::FieldContent)->addText(toString());
    return document;
}

}
