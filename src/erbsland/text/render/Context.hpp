// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Context_fwd.hpp"
#include "Value.hpp"

namespace erbsland::text::render {

/// A named collection of values exposed while rendering a layout.
/// @seedoc{/reference/text/render}
/// @tested{RenderContextTest}
class Context final {
public:
    /// Create an empty context.
    Context() = default;
    /// Create a context from named values.
    explicit Context(ValueMap values) noexcept : _values{std::move(values)} {}

    // defaults
    ~Context() = default;
    Context(const Context &) noexcept = default;
    Context(Context &&) noexcept = default;
    auto operator=(const Context &) noexcept -> Context & = default;
    auto operator=(Context &&) noexcept -> Context & = default;

public:
    /// Test whether a value with `name` exists.
    [[nodiscard]] auto contains(const String &name) const noexcept -> bool { return _values.contains(name); }
    /// Get a named value, or null when it does not exist.
    [[nodiscard]] auto get(const String &name) const -> Value { return _values.get(name, Value{}); }
    /// Insert or replace a named value.
    auto set(const String &name, Value value) -> Context & {
        _values.set(name, std::move(value));
        return *this;
    }
    /// Access all named values.
    [[nodiscard]] auto values() const noexcept -> const ValueMap & { return _values; }

private:
    ValueMap _values; ///< Named values in this context.
};

}
