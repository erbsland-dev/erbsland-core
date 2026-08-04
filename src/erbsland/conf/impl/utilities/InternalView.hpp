// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "InternalView_fwd.hpp"

#include "../../../err/LogicError.hpp"
#include "../../../text/Literals.hpp"
#include "../../../text/StringEditor.hpp"
#include "../../../text/StringFormat.hpp"
#include "../../../text/StringList.hpp"
#include "../../../text/u8/impl/U8StringLiteralFactory.hpp"

#include <memory>
#include <variant>
#include <vector>

namespace erbsland::conf::impl {

using namespace text::literals;

/// Get the internal view of an object.
template <typename T>
auto internalView(const T & /*object*/) -> InternalViewPtr {
    // uncomment for active development:
    static_assert(false, "missing specialization");
    throw err::LogicError("Not implemented"_el);
}

/// The internal view structure for testing and debugging.
/// This class helps debugging and testing this parser by providing insights into the internals, without
/// actually changing the functionality of the objects. By having a friend function `internalView`, gathering
/// internals works like a regular text conversion helper and allows implementing safe access to internals, without the
/// risk after altering its functionality in release builds. As an extra safety measure, the macro
/// `ERBSLAND_CORE_CONF_INTERNAL_VIEW` is in place to remove these functions entirely from any production build.
/// @tested{LexerTokenTest NamePathTest NameTest}
class InternalView {
    /// Restrict direct construction to this class.
    class PrivateTag {};

private:
    /// Store either rendered text or a nested internal view.
    using Value = std::variant<text::String, InternalViewPtr>;

public:
    /// Create an empty internal view.
    explicit InternalView(PrivateTag) noexcept {}
    /// Create a named internal view.
    InternalView(const text::String &name, const Value &value, PrivateTag) noexcept;
    /// Create an empty shared internal view.
    [[nodiscard]] static auto create() noexcept -> InternalViewPtr;
    /// Create a named shared internal view.
    [[nodiscard]] static auto create(const text::String &name, const Value &value) noexcept -> InternalViewPtr;

    /// Create a view to a list of objects.
    /// @tparam Iter The iterator type
    /// @param maxElements The maximum number of elements or 0 for no limit.
    /// @param begin The begin iterator to the list.
    /// @param end The end iterator to the list.
    /// @return The view instance.
    template <typename Iter>
    [[nodiscard]] static auto createList(const std::size_t maxElements, const Iter &begin, const Iter &end) noexcept
        -> InternalViewPtr {

        auto result = std::make_shared<InternalView>(PrivateTag{});
        result->setValue("size", std::distance(begin, end));
        std::size_t index = 0;
        for (auto it = begin; it != end; ++it) {
            result->setValue(text::StringFormat{"{:04}"_el}.build(index++), internalView(*it));
            if (maxElements > 0 && index >= maxElements) {
                break;
            }
        }
        return result;
    }
    /// Create a view to a list of objects.
    /// @tparam Iter The iterator type
    /// @param begin The begin iterator to the list.
    /// @param end The end iterator to the list.
    /// @param nameFunc Creates the display name for each list element.
    /// @return The view instance.
    template <typename Iter>
    [[nodiscard]] static auto createNamedList(
        const Iter &begin,
        const Iter &end,
        const std::function<text::String(const typename Iter::value_type &)> &nameFunc) noexcept -> InternalViewPtr {

        auto result = std::make_shared<InternalView>(PrivateTag{});
        for (auto it = begin; it != end; ++it) {
            result->setValue(nameFunc(*it), internalView(*it));
        }
        return result;
    }
    /// Remove the value named `name`.
    void removeValue(const text::String &name) noexcept;
    /// Store an internal value under `name`.
    void setValue(const text::String &name, const Value &value) noexcept;
    /// Store text under `name`.
    void setValue(const text::String &name, const text::String &value) noexcept;
    /// Store a nested view under `name`.
    void setValue(const text::String &name, const InternalViewPtr &value) noexcept;
    /// Store raw text under `name` without further formatting.
    void setUnsafeText(
        const text::String &name, const text::String &text, const text::String &textIfEmpty = text::String{});
    template <std::size_t N>
    /// Store a UTF-8 literal under `name`.
    void setValue(const text::String &name, const char8_t (&literal)[N]) noexcept {
        setValue(name, text::String{text::impl::createU8StringLiteral(literal, N - 1)});
    }
    template <std::size_t N, typename T>
    /// Store a value under an ASCII literal name.
    void setValue(const char (&name)[N], T &&value) noexcept {
        setValue(text::String{text::impl::createU8StringLiteral(name, N - 1)}, std::forward<T>(value));
    }
    template <std::size_t N, typename T>
    /// Store a value under a UTF-8 literal name.
    void setValue(const char8_t (&name)[N], T &&value) noexcept {
        setValue(text::String{text::impl::createU8StringLiteral(name, N - 1)}, std::forward<T>(value));
    }
    template <std::size_t N>
    /// Store raw text under an ASCII literal name.
    void setUnsafeText(const char (&name)[N], const text::String &value) noexcept {
        setUnsafeText(text::String{text::impl::createU8StringLiteral(name, N - 1)}, value);
    }
    template <std::size_t N>
    /// Store raw text under a UTF-8 literal name.
    void setUnsafeText(const char8_t (&name)[N], const text::String &value) noexcept {
        setUnsafeText(text::String{text::impl::createU8StringLiteral(name, N - 1)}, value);
    }
    /// Store a Boolean value under `name`.
    void setValue(const text::String &name, const bool value) noexcept;
    template <typename T>
        requires(std::is_integral_v<T>)
    /// Store an integral value under `name`.
    void setValue(const text::String &name, const T &value) noexcept {
        setValue(name, text::String{text::StringEditor::fromInteger(value)});
    }
    template <typename T>
        requires(!std::integral<std::remove_cvref_t<T>> && !std::convertible_to<T, text::String>)
    /// Store the internal view of `value` under `name`.
    void setValue(const text::String &name, const T &value) noexcept {
        setValue(name, internalView(value));
    }
    /// Render this view with `indent` spaces.
    [[nodiscard]] auto toString(const std::size_t indent = 0) const noexcept -> text::String;

private:
    /// Render this view as individual indented lines.
    [[nodiscard]] auto toLines(const std::size_t indent = 0) const noexcept -> text::StringList;

private:
    std::vector<std::pair<text::String, Value>> _values;
};

}
