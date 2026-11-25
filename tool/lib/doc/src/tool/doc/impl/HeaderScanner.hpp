// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/StringList.hpp>

#include <utility>
#include <vector>

namespace tool::doc::impl {

/// Discover documented namespace-scope declarations for Breathe reference interfaces.
class HeaderScanner final {
    /// Declaration state carried between source lines during one scan.
    struct ScanState {
        std::vector<std::pair<int, el::String>> namespaces; ///< Active namespace names and their brace depths.
        int depth{};                                        ///< Current tokenized brace nesting.
        bool documented{};                                  ///< A namespace-scope API comment is pending.
        bool functionReturn{};                              ///< A standalone function return type was encountered.
        el::String pendingFunction;                         ///< Accumulated multiline function signature.
        el::StringList result;                              ///< Rendered directives in source order.
    };

public:
    // defaults
    HeaderScanner() = default;

public: // scanning
    /// Scan a complete header into Breathe directive blocks.
    /// @param source Header text, including documentation comments.
    /// @return Supported documented declarations in source order.
    auto scan(const el::String &source) const -> el::StringList;

private:
    /// Report a failure in this operation, optionally identifying its input file.
    [[noreturn]] static void throwError(const el::String &description, const el::String &path = {});

private:
    /// Count braces outside comments, literals, and preprocessing directives per source line.
    static auto braceDeltas(const el::String &source, el::ItemCount lineCount) -> std::vector<int>;
    /// Advance namespace and declaration state for one non-documentation line.
    static void scanLine(ScanState &state, const el::String &current);
    /// Recognize a documented type or function within its active namespace.
    static void scanDocumented(ScanState &state, const el::String &current, const el::StringList &scope);
    /// Normalize only the whitespace needed in a function signature.
    static auto signature(const el::String &text) -> el::String;
};

}
