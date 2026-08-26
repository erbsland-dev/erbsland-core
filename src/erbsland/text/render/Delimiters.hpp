// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../String.hpp"

namespace erbsland::text::render {

/// One configurable pair of layout delimiters.
/// @tested{RenderEnvironmentTest}
class Delimiters final {
public:
    /// Create a new set of delimiters.
    Delimiters(String begin, String end, String line = {}) :
        _begin{std::move(begin)}, _end{std::move(end)}, _line{std::move(line)} {}

    // defaults
    ~Delimiters() = default;
    Delimiters(const Delimiters &) = default;
    Delimiters(Delimiters &&) = default;
    auto operator=(const Delimiters &) -> Delimiters & = default;
    auto operator=(Delimiters &&) -> Delimiters & = default;

public:
    /// Get the begin delimiter
    [[nodiscard]] auto begin() const noexcept -> const String & { return _begin; }
    /// Get the end delimiter
    [[nodiscard]] auto end() const noexcept -> const String & { return _end; }
    /// Get the line delimiter
    [[nodiscard]] auto line() const noexcept -> const String & { return _line; }

private:
    String _begin; ///< The begin delimiter, like `{%`. At least one code point.
    String _end;   ///< The end delimiter, like `%}`. At least one code point.
    String _line;  ///< An optional delimiter for line statements, like `%%`. Empty if not used.
};

}
