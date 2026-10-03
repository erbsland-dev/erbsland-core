// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "impl/Reader_fwd.hpp"

#include "../NamePath.hpp"
#include "../ValueType.hpp"

#include "../../unit/ByteRange.hpp"
#include "../../unit/ItemIndex.hpp"

#include <optional>

namespace erbsland::conf::syntax {

/// One section declaration or assignment, including all its continued lines.
/// @tested{SyntaxParserTest SyntaxFormatterTest}
class Statement final {
    friend class impl::Reader;

public:
    /// Statement categories.
    enum class Kind { Section, Assignment }; ///< Source-level statement forms.
    /// Get the statement category.
    [[nodiscard]] auto kind() const noexcept -> Kind { return _kind; }
    /// Test whether formatting must preserve this statement verbatim.
    [[nodiscard]] auto hasErrors() const noexcept -> bool { return _hasErrors; }
    /// Test whether this is a list section.
    [[nodiscard]] auto isListSection() const noexcept -> bool { return _listSection; }
    /// Test whether the section path was relative.
    [[nodiscard]] auto isRelativeSection() const noexcept -> bool { return _relative; }
    /// Test whether this is a meta assignment.
    [[nodiscard]] auto isMetaValue() const noexcept -> bool { return _meta; }
    /// Get the number of names written in a section header.
    [[nodiscard]] auto sectionNameCount() const noexcept -> std::size_t { return _nameCount; }
    /// Get the locally resolved path, when available.
    [[nodiscard]] auto namePath() const noexcept -> const std::optional<NamePath> & { return _path; }
    /// Get the recognized assignment value type.
    [[nodiscard]] auto valueType() const noexcept -> ValueType { return _valueType; }
    /// Get the original source range including its final newline.
    [[nodiscard]] auto range() const noexcept -> unit::ByteRange { return _range; }
    /// Get the first token index.
    [[nodiscard]] auto firstToken() const noexcept -> unit::ItemIndex { return _firstToken; }
    /// Get the exclusive last token index.
    [[nodiscard]] auto endToken() const noexcept -> unit::ItemIndex { return _endToken; }

private:
    Kind _kind{Kind::Assignment};  ///< Statement form.
    bool _hasErrors{false};        ///< Unsafe to rewrite.
    bool _listSection{false};      ///< List header flag.
    bool _relative{false};         ///< Relative path flag.
    bool _meta{false};             ///< Meta assignment flag.
    std::size_t _nameCount{};      ///< Names in the written section path.
    std::optional<NamePath> _path; ///< Best-effort declaration path.
    ValueType _valueType;          ///< Recognized value type.
    unit::ByteRange _range;        ///< Source range.
    unit::ItemIndex _firstToken{}; ///< First token.
    unit::ItemIndex _endToken{};   ///< Exclusive token end.
};

}
