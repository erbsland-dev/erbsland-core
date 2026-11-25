// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ApiGuidelineState.hpp"

#include <erbsland/ItemIndex.hpp>
#include <erbsland/String.hpp>
#include <erbsland/StringList.hpp>

namespace tool::doc::impl {

/// Validate the structure and grammar of one API guideline page.
class ApiGuidelines final {

public:
    // defaults
    ApiGuidelines() = default;

public: // validation
    /// Validate one page, collecting source-line diagnostics.
    /// @param text RST page text.
    /// @return All discovered issues.
    auto validate(const el::String &text) const -> el::StringList;

private:
    /// Validate document limits and the opening title, stopping on an incomplete title.
    static auto validateTitle(const el::StringList &lines, ApiGuidelineState &state) -> bool;
    /// Close the preceding section and validate the next section transition.
    static void openSection(ApiGuidelineState &state, const el::String &current, el::ItemIndex index);
    /// Validate one content line against its section grammar.
    void validateContent(ApiGuidelineState &state, const el::StringList &lines, el::ItemIndex index) const;
    /// Validate a single code-section entry.
    auto codeLine(const el::String &text, const el::String &kind) const -> el::String;
    /// Test a return type, including nested template arguments.
    auto returnType(el::String value) const -> bool;
    /// Validate a type list or slash-separated alternatives.
    auto typeList(const el::String &value, bool allowSlash) const -> bool;
    /// Validate constructors, method alternatives, and free-function patterns.
    auto callPattern(const el::String &value) const -> bool;
};

}
