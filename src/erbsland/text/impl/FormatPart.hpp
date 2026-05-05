// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "FormatPartKind.hpp"
#include "FormatSpec.hpp"

#include "../AnyString.hpp"

#include "../../unit/ArgumentUnit.hpp"

namespace erbsland::text::impl {

/// One compiled UTF-8 format part.
/// @tested{U8FormatTest}
class FormatPart final {
public:
    // defaults
    FormatPart() = default;
    ~FormatPart() = default;
    FormatPart(const FormatPart &) = default;
    FormatPart(FormatPart &&) = default;
    auto operator=(const FormatPart &) -> FormatPart & = default;
    auto operator=(FormatPart &&) -> FormatPart & = default;

public: // accessors
    [[nodiscard]] auto kind() const -> FormatPartKind { return _kind; }
    [[nodiscard]] auto text() const -> const AnyString & { return _text; }
    [[nodiscard]] auto argumentIndex() const -> unit::ArgumentIndex { return _argumentIndex; }
    [[nodiscard]] auto spec() const -> const FormatSpec & { return _spec; }
    [[nodiscard]] auto spec() -> FormatSpec & { return _spec; }

public: // modifiers
    auto setArgumentIndex(const unit::ArgumentIndex argumentIndex) noexcept -> FormatPart & {
        _argumentIndex = argumentIndex;
        return *this;
    }

public: // factories
    [[nodiscard]] static auto fromStaticText(AnyString text) -> FormatPart {
        FormatPart part;
        part._text = std::move(text);
        part._kind = FormatPartKind::StaticText;
        return part;
    }
    [[nodiscard]] static auto fromField(const FormatSpec &spec, const unit::ArgumentIndex argumentIndex) -> FormatPart {
        FormatPart part;
        part._kind = FormatPartKind::Field;
        part._spec = spec;
        part._argumentIndex = argumentIndex;
        return part;
    }

private:
    FormatPartKind _kind{FormatPartKind::Field};                     ///< The kind of format part.
    AnyString _text;                                                 ///< Static text.
    unit::ArgumentIndex _argumentIndex{unit::ArgumentIndex::zero()}; ///< The argument index for fields.
    FormatSpec _spec{};                                              ///< The field format specification.
};

}
