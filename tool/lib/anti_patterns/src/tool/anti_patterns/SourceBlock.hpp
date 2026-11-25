// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/ByteRange.hpp>
#include <erbsland/ItemIndex.hpp>
#include <erbsland/String.hpp>

namespace tool::anti_patterns {

/// One balanced C++ brace scope in the shared lexical index.
struct SourceBlock final {
    /// Broad declaration category for this brace scope.
    enum class Kind : uint8_t {
        Namespace,
        Class,
        Struct,
        EnumClass,
        Other,
    };

    el::ByteRange braces;                            ///< Brace span; an unclosed scope ends one byte beyond EOF.
    el::ByteIndex headerStart;                       ///< First byte after the preceding structural delimiter.
    el::ItemIndex parent = el::ItemIndex::noIndex(); ///< Parent block index, or noIndex().
    Kind kind{};                                     ///< Heuristic declaration category.
    el::String name;                                 ///< Namespace or type name, when recognized.

    /// Get the opening brace position for lexical heuristics.
    [[nodiscard]] auto openingBrace() const noexcept -> el::ByteIndex { return braces.beginIndex(); }
    /// Get the closing brace position, or EOF for an unclosed scope.
    [[nodiscard]] auto closingBrace() const noexcept -> el::ByteIndex { return braces.lastIndex(); }
};

}
