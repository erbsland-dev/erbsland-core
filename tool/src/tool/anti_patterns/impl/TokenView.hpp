// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../SourceIndex.hpp"

#include <initializer_list>

namespace tool::anti_patterns::impl {

/// Comment-free tokens retaining literals and preprocessing barriers for expression checks.
class TokenView final {
public:
    /// Index the source tokens, omitting only comments.
    /// @param source The immutable source snapshot.
    explicit TokenView(const SourceIndex &source);

public: // queries
    /// Get the number of retained tokens.
    [[nodiscard]] auto count() const -> el::ItemCount { return _tokens.count(); }
    /// Read a token, or an end token for an invalid index.
    /// @param index Position in the comment-free token sequence.
    /// @return The token or the shared end sentinel.
    [[nodiscard]] auto at(el::ItemIndex index) const -> const el::cpp::CppToken &;
    /// Match consecutive token spellings.
    /// @param index First token to inspect.
    /// @param words Expected token spellings.
    /// @return Whether all spellings match.
    [[nodiscard]] auto matches(el::ItemIndex index, std::initializer_list<el::String> words) const -> bool;
    /// Find a balanced closing delimiter, stopping at preprocessing directives.
    /// @param opening Index of the opening delimiter.
    /// @return The closing index, or no-index for an incomplete or mismatched expression.
    [[nodiscard]] auto closing(el::ItemIndex opening) const -> el::ItemIndex;
    /// Match an unqualified or Core-qualified type and return the following token.
    /// @param index First token of the candidate type name.
    /// @param name The unqualified Core type name.
    /// @param domain The owning Core domain.
    /// @return The token after the type name, or no-index when it does not match.
    [[nodiscard]] auto afterType(el::ItemIndex index, const el::String &name, const el::String &domain) const
        -> el::ItemIndex;

private:
    el::List<el::cpp::CppToken> _tokens; ///< Tokens with their original source coordinates.
    el::cpp::CppToken _end;              ///< Safe out-of-bounds sentinel.
};

}
