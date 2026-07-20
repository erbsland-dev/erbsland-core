// Copyright (c) 2025 Erbsland DEV. https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Name.hpp"

#include "impl/utilities/InternalView.hpp"

#include <algorithm>
#include <limits>
#include <ranges>
#include <vector>

namespace erbsland::conf {

/// A name-path.
/// This class represents a name path that points to elements in a configuration document. It allows building
/// paths freely, using individual name elements.
/// Note that, unlike in configuration documents where relative and absolute paths differ by a leading separator,
/// both forms use the same text representation in this API. This lack of differentiation exists because the
/// API treats both forms the same.
/// Addressing a value is always done using relative paths, and the element on which you call the "value" method
/// decides if you start to resolve the path from the root or from a branch in the document.
/// @tested{NamePathTest NamePathLexerTest ValueNamingTest}
class NamePath final {
public:
    // Required definitions to be used with std algorithms.
    // Only provide const access to the elements of a name path.
    /// Iterator over the names in the path.
    using iterator = NameList::const_iterator;
    /// Constant iterator over the names in the path.
    using const_iterator = NameList::const_iterator;
    /// The element type of the name path.
    using value_type = Name;
    /// Reference to a name element.
    using reference = const Name &;
    /// Constant reference to a name element.
    using const_reference = const Name &;
    /// Unsigned integer type for sizes.
    using size_type = std::size_t;
    /// Type used for indexing elements.
    using Index = std::size_t;
    /// Type used for element counts.
    using Count = std::size_t;
    /// Signed integer type for differences.
    using difference_type = std::ptrdiff_t;
    /// Pointer to a name element.
    using pointer = const Name *;
    /// Constant pointer to a name element.
    using const_pointer = const Name *;
    /// Reverse iterator over the names in the path.
    using reverse_iterator = std::reverse_iterator<iterator>;
    /// Constant reverse iterator over the names in the path.
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;
    /// Category type of constant iterators.
    using const_iterator_category = std::forward_iterator_tag;
    /// Category type of iterators.
    using iterator_category = std::forward_iterator_tag;

    /// Marker returned by search functions when no element was found.
    static constexpr Index npos = std::numeric_limits<Index>::max();

public:
    /// Create a name path with one single name.
    /// @param name The name.
    NamePath(const Name &name) : _names{1, name} {} // NOLINT(*-explicit-constructor)
    /// Create a name path with one single name.
    /// @param name The name.
    NamePath(Name &&name) { // NOLINT(*-explicit-constructor)
        _names.emplace_back(std::move(name));
    }
    /// Create a name path from the given sequence of names.
    /// @param names A list of names.
    explicit NamePath(NameList names) : _names{std::move(names)} {}
    /// Create a name path from the given sequence of names.
    /// @param names A span of names.
    explicit NamePath(const std::span<const Name> names) : _names{names.begin(), names.end()} {}
    /// Create a name path from the given sequence of names.
    /// @param begin The start iterator.
    /// @param end The stop iterator.
    explicit NamePath(const NameList::const_iterator begin, const NameList::const_iterator end) : _names{begin, end} {}

    // defaults
    NamePath() = default;
    ~NamePath() = default;

public: // operators
    /// Default comparison.
    auto operator<=>(const NamePath &) const -> std::strong_ordering = default;
    /// Default comparison.
    auto operator==(const NamePath &) const -> bool = default;

public: // accessors and tests.
    /// Test if this is an empty path.
    [[nodiscard]] auto empty() const noexcept -> bool;
    /// Get the number of elements in this path.
    [[nodiscard]] auto size() const noexcept -> Count;
    /// Access one name in the name path.
    [[nodiscard]] auto at(Index index) const -> const Name &;
    /// Find the first element that equals the given name.
    /// @param name The name to search for.
    /// @return The index of the first matching element, or `npos` if there is no match.
    [[nodiscard]] auto find(const Name &name) const noexcept -> Index;
    /// Access the first element.
    [[nodiscard]] auto front() const -> const Name &;
    /// Access the last element.
    [[nodiscard]] auto back() const -> const Name &;

public: // tests
    /// Test if this path contains an index (index or text-index).
    /// @return `true` if this path contains any index or text-index element, `false` otherwise.
    [[nodiscard]] auto containsIndex() const noexcept -> bool;
    /// Test if this path contains a text-name.
    /// @return `true` if this path contains a text-name element, `false` otherwise.
    [[nodiscard]] auto containsText() const noexcept -> bool;

public: // iterators
    /// Get an iterator to the first name in the path.
    [[nodiscard]] auto begin() const noexcept -> NameList::const_iterator;
    /// Get an iterator to the end of the path.
    [[nodiscard]] auto end() const noexcept -> NameList::const_iterator;
    /// Access a view of all elements.
    [[nodiscard]] auto view() const noexcept -> std::span<const Name>;

public: // working with the name path.
    /// Get the parent path.
    [[nodiscard]] auto parent() const noexcept -> NamePath;
    /// Return a sub-path from this path.
    /// If the given index range is invalid, it returns an empty path.
    /// @param pos The start index.
    /// @param count The maximum number of elements to include.
    /// @return The resulting sub-path.
    [[nodiscard]] auto subPath(Index pos = 0, Count count = npos) const noexcept -> NamePath;
    /// Append a name to this path.
    /// @param name The name to append to this path.
    template <typename Fwd>
        requires std::is_convertible_v<Fwd, Name>
    void append(Fwd &&name) noexcept {
        _names.emplace_back(std::forward<Fwd>(name));
    }
    /// Append another name path to this path.
    /// @param namePath The name path to append.
    void append(const NamePath &namePath) noexcept;
    /// Prepend another name path in front of this path.
    /// @param namePath The name path to prepend.
    void prepend(const NamePath &namePath) noexcept;
    /// Remove the last element of this path.
    void popBack() noexcept;
    /// Clear the path.
    void clear() noexcept;

public: // conversion
    /// Convert this name path into a string.
    /// @return The name path in text form, or an empty string for an empty path or if the path is not valid.
    [[nodiscard]] auto toText() const noexcept -> text::String;
    /// Convert a name path from a text.
    /// Convert name paths for accessing value elements, therefore, it supports the name path extensions for the API.
    /// Create a name path from the given text: e.g. "main.server[2].path", or just "main",
    /// "[1]", "\"text\"", "\"\"[1]" as the path may start at any value element in the value-tree.
    /// @param text The path in its text form.
    /// @throws ConfError (Syntax, Encoding, Limit) In case of any problem with the name.
    [[nodiscard]] static auto fromText(const text::String &text) -> NamePath;

public:
#ifdef ERBSLAND_CORE_CONF_INTERNAL_VIEWS
    friend auto internalView(const NamePath &object) -> impl::InternalViewPtr;
#endif

private:
    NameList _names; ///< The list of names that build the name path.
};

/// A name-path or convertible value.
using NamePathLike = std::variant<Name, NamePath, text::String, std::size_t>;

/// A list of name paths.
using NamePathList = std::vector<NamePath>;

/// Convert a name-path like value into a name path.
[[nodiscard]] auto toNamePath(const NamePathLike &namePathLike) -> NamePath;

}

template <>
struct std::hash<erbsland::conf::NamePath> {
    auto operator()(const erbsland::conf::NamePath &namePath) const noexcept -> std::size_t {
        std::size_t result = 0;
        for (const auto &name : namePath) {
            erbsland::util::advanceHash(result, name);
        }
        return result;
    }
};
