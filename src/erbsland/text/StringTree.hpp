// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "IntegerFormat.hpp"
#include "String.hpp"
#include "StringEditor.hpp"
#include "StringTree_fwd.hpp"

#include "impl/StringTreeData.hpp"

#include "../math/IntegerTraits.hpp"
#include "../unit/CpLength.hpp"

#include <concepts>
#include <ranges>

namespace erbsland::text {

/// A small structured text tree for diagnostics and debug views.
/// @tested{StringTreeTest}
class StringTree final {
public:
    /// Create an empty tree.
    StringTree();
    /// Create a tree with a title.
    explicit StringTree(String title);

    // defaults
    ~StringTree() = default;
    StringTree(const StringTree &) = default;
    StringTree(StringTree &&) = default;
    auto operator=(const StringTree &) -> StringTree & = default;
    auto operator=(StringTree &&) -> StringTree & = default;

public: // accessors
    /// Test if this tree has no title and no entries.
    [[nodiscard]] auto isEmpty() const noexcept -> bool;
    /// Access the title.
    [[nodiscard]] auto title() const noexcept -> String;

public: // modifiers
    /// Set the tree title.
    auto setTitle(String title) -> StringTree &;
    /// Append one text line.
    auto append(String text) -> StringTree &;
    /// Append a labeled string value.
    auto append(String label, String value) -> StringTree &;
    /// Append a labeled boolean value.
    auto append(String label, bool value) -> StringTree &;
    /// Append a labeled integer value.
    template <math::AnyIntegerType T>
    auto append(String label, T value, IntegerFormat format = IntegerFormat::defaultFormat()) -> StringTree &;
    /// @cond INTERNAL
    /// Append a labeled value with a `toRawValue()` method.
    template <typename T>
        requires requires(const T &value) { value.toRawValue(); } && (!math::AnyIntegerType<T>)
    auto append(String label, const T &value, IntegerFormat format = IntegerFormat::defaultFormat()) -> StringTree &;
    /// @endcond
    /// Append a labeled subtree.
    auto append(String label, const StringTree &tree) -> StringTree &;
    /// Append a labeled subtree.
    auto append(String label, StringTree &&tree) -> StringTree &;
    /// Append all entries from another tree.
    auto append(const StringTree &tree) -> StringTree &;
    /// Append an indexed list.
    template <std::ranges::input_range Range, typename Convert>
    auto appendList(String label, Range &&range, Convert convert) -> StringTree &;

public: // conversion
    /// Convert the tree into formatted text.
    [[nodiscard]] auto toString(
        unit::CpLength indentWidth = unit::CpLength{4U},
        unit::CpLength initialIndentWidth = unit::CpLength::zero()) const -> String;

private:
    explicit StringTree(impl::StringTreeDataPtr data);
    [[nodiscard]] static auto createData(String title = {}) -> impl::StringTreeDataPtr;
    static void appendData(
        StringEditor &result,
        const impl::StringTreeData &data,
        unit::CpLength indentWidth,
        unit::CpLength indent,
        bool &firstLine);
    static void appendEntry(
        StringEditor &result,
        const impl::StringTreeEntry &entry,
        unit::CpLength indentWidth,
        unit::CpLength indent,
        bool &firstLine);
    static void appendLinePrefix(StringEditor &result, unit::CpLength indent, bool &firstLine);
    [[nodiscard]] static auto listIndexLabel(std::size_t index) -> String;
    void ensureUnique();

private:
    impl::StringTreeDataPtr _data;
};

}

#include "StringTree.tpp"
