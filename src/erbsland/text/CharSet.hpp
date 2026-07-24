// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "AsciiCategory.hpp"
#include "CharRange.hpp"
#include "CharSet_fwd.hpp"
#include "StringEditor_fwd.hpp"
#include "UnicodeCategory.hpp"

#include "u16/U16String_fwd.hpp"
#include "u16/U16StringEditor_fwd.hpp"
#include "u32/U32String_fwd.hpp"
#include "u32/U32StringEditor_fwd.hpp"
#include "u8/U8String_fwd.hpp"
#include "u8/U8StringEditor_fwd.hpp"

#include "../mem/SharedArrayData_fwd.hpp"
#include "../mem/SharedDataPointer.hpp"
#include "../util/List.hpp"
#include "../util/LoopResult.hpp"
#include "../util/LoopStatus.hpp"
#include "../util/Set.hpp"

#include <array>
#include <concepts>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <optional>
#include <span>
#include <type_traits>
#include <utility>
#include <variant>

namespace erbsland::text {

namespace impl {
class CharSetRangeBuilder;
}

/// A normalized set of Unicode scalar values.
/// The set stores up to two ranges inline and uses copy-on-write storage for larger sets. Invalid characters are
/// ignored.
/// @seedoc{/reference/text/char_range}
/// @tested{CharSetTest}
class CharSet final {
private:
    struct InlineRanges {
        std::array<CharRange, 2> values{}; ///< The inline normalized ranges.
    };

    using RangeData = mem::SharedArrayData<
        CharRange,
        uint32_t,
        mem::SharedArrayDataConstructMethod::None,
        mem::SharedArrayDataCleanupMethod::None>;
    using RangeDataPtr = mem::SharedDataPointer<RangeData, true>;
    using Storage = std::variant<InlineRanges, RangeDataPtr>;

    static_assert(std::is_trivially_copyable_v<CharRange>);

public:
    /// Create an empty character set.
    CharSet() = default;
    /// Create a character set containing one character.
    explicit CharSet(Char character);
    /// Decode a UTF-8 view tolerantly into a character set.
    explicit CharSet(const U8String &characters);
    /// Create a character set from an ordered Erbsland set of characters.
    explicit CharSet(const util::Set<Char> &characters);
    /// Create a character set from an Erbsland list of characters.
    explicit CharSet(const util::List<Char> &characters);
    /// Create a character set from a list of characters.
    CharSet(std::initializer_list<Char> characters);

    // defaults
    ~CharSet();
    CharSet(const CharSet &) noexcept;
    CharSet(CharSet &&other) noexcept;
    auto operator=(const CharSet &) noexcept -> CharSet &;
    auto operator=(CharSet &&other) noexcept -> CharSet &;

public: // operators
    auto operator==(const CharSet &other) const noexcept -> bool;
    auto operator!=(const CharSet &other) const noexcept -> bool { return !operator==(other); }
    auto operator<=(const CharSet &other) const -> bool { return isSubsetOf(other); }
    auto operator>=(const CharSet &other) const -> bool { return isSupersetOf(other); }
    auto operator|(const CharSet &other) const -> CharSet { return unitedWith(other); }
    auto operator&(const CharSet &other) const -> CharSet { return intersectedWith(other); }
    auto operator-(const CharSet &other) const -> CharSet { return subtractedBy(other); }
    auto operator^(const CharSet &other) const -> CharSet { return symmetricDifferenceWith(other); }
    auto operator|=(const CharSet &other) -> CharSet &;
    auto operator&=(const CharSet &other) -> CharSet &;
    auto operator-=(const CharSet &other) -> CharSet &;
    auto operator^=(const CharSet &other) -> CharSet &;

public: // tests
    /// Test if this set is empty.
    [[nodiscard]] auto isEmpty() const noexcept -> bool { return rangeSpan().empty(); }
    /// Test if the character is contained in this set.
    [[nodiscard]] auto contains(Char character) const noexcept -> bool;
    /// Test if this set is a subset of another set.
    [[nodiscard]] auto isSubsetOf(const CharSet &other) const -> bool;
    /// Test if this set is a superset of another set.
    [[nodiscard]] auto isSupersetOf(const CharSet &other) const -> bool { return other.isSubsetOf(*this); }
    /// Test if this set equals another set after Unicode simple case folding.
    /// @usesunidb{Uses generated Unicode Character Database character metadata.}
    [[nodiscard]] auto isEqualToCI(const CharSet &other) const -> bool;
    /// Test if this set is a subset of another set after Unicode simple case folding.
    /// @usesunidb{Uses generated Unicode Character Database character metadata.}
    [[nodiscard]] auto isSubsetOfCI(const CharSet &other) const -> bool;
    /// Test if this set contains characters affected by Unicode simple case folding.
    /// @usesunidb{Uses generated Unicode Character Database character metadata.}
    [[nodiscard]] auto containsCaseFoldableCharacters() const -> bool;
    /// Test if this set contains characters affected by Unicode simple lowercase mapping.
    /// @usesunidb{Uses generated Unicode Character Database character metadata.}
    [[nodiscard]] auto containsLowercaseMappableCharacters() const -> bool;
    /// Test if this set contains characters affected by Unicode simple uppercase mapping.
    /// @usesunidb{Uses generated Unicode Character Database character metadata.}
    [[nodiscard]] auto containsUppercaseMappableCharacters() const -> bool;

public: // accessors
    /// Create the union of this set and another set.
    [[nodiscard]] auto unitedWith(const CharSet &other) const -> CharSet;
    /// Create the intersection of this set and another set.
    [[nodiscard]] auto intersectedWith(const CharSet &other) const -> CharSet;
    /// Create this set without another set.
    [[nodiscard]] auto subtractedBy(const CharSet &other) const -> CharSet;
    /// Create the symmetric difference of this set and another set.
    [[nodiscard]] auto symmetricDifferenceWith(const CharSet &other) const -> CharSet;

public: // modification
    /// Add another set to this set.
    void add(const CharSet &other);
    /// Add one range to this set.
    void add(CharRange range);
    /// Add one character to this set.
    void add(Char character);
    /// Remove another set from this set.
    void remove(const CharSet &other);
    /// Remove one range from this set.
    void remove(CharRange range);
    /// Remove one character from this set.
    void remove(Char character);

public: // conversion
    /// Iterate over all ranges or characters in this set.
    /// If the function accepts a `CharRange`, ranges are iterated. Otherwise, if it accepts a `Char`, all Unicode
    /// scalar values are iterated in ascending order. If the function returns `LoopStatus`, `Stop` or `Error` stops
    /// iteration.
    template <typename Function>
    auto forEach(Function function) const -> util::LoopResult;
    /// Transform all characters in this set and return a normalized transformed set.
    template <typename Function>
    [[nodiscard]] auto transform(Function function) const -> CharSet;
    /// Return the simple case-folded form of this set.
    /// @usesunidb{Uses generated Unicode Character Database character metadata.}
    [[nodiscard]] auto caseFolded() const -> CharSet;
    /// Convert this set to lowercase.
    /// @usesunidb{Uses generated Unicode Character Database character metadata.}
    [[nodiscard]] auto toLowercase() const -> CharSet;
    /// Convert this set to uppercase.
    /// @usesunidb{Uses generated Unicode Character Database character metadata.}
    [[nodiscard]] auto toUppercase() const -> CharSet;
    /// Export all characters as a UTF-8 string.
    [[nodiscard]] auto toString() const -> String;
    /// Export all characters as a UTF-8 string.
    [[nodiscard]] auto toU8String() const -> U8String;
    /// Export all characters as a UTF-16 string.
    [[nodiscard]] auto toU16String() const -> U16String;
    /// Export all characters as a UTF-32 string.
    [[nodiscard]] auto toU32String() const -> U32String;
    /// Export all characters as an ordered Erbsland set.
    [[nodiscard]] auto toSet() const -> util::Set<Char>;
    /// Export all characters as an Erbsland list in ascending code-point order.
    [[nodiscard]] auto toList() const -> util::List<Char>;

public: // factory methods
    /// Create a character set containing one character range.
    [[nodiscard]] static auto fromRange(Char from, Char to) -> CharSet;
    /// Create a character set from an ASCII-only category.
    [[nodiscard]] static auto from(AsciiCategory category) -> CharSet;
    /// Create a character set from a Unicode general category.
    /// @usesunidb{Uses generated Unicode Character Database character metadata.}
    [[nodiscard]] static auto from(UnicodeCategory category) -> CharSet;
    /// @overload
    [[nodiscard]] static auto from(UnicodeCategoryGroup categoryGroup) -> CharSet;
    /// Create a character set from a regexp like pattern.
    /// The hypen character in the pattern defines ranges in the form `<first>-<last>`.
    /// The code point of `<first>` must be before `<last>`.
    /// The hypen character at the beginning or end of the pattern is treated as a literal character.
    /// Duplicated characters and ranges are ignored.
    /// Example: `fromPattern("-a-f_0-9=/")` characters `-_=/` and ranges `a-f` and `0-9`.
    /// @param pattern The pattern string to parse.
    /// @throws err::ParseError For an invalid pattern syntax.
    [[nodiscard]] static auto fromPattern(const U8String &pattern) -> CharSet;
    /// @overload
    [[nodiscard]] static auto fromPattern(const U16String &pattern) -> CharSet;
    /// @overload
    [[nodiscard]] static auto fromPattern(const U32String &pattern) -> CharSet;

private:
    friend class impl::CharSetRangeBuilder;

    template <typename>
    static constexpr auto cIsSupportedForEachFunction = false;

    template <typename Function, typename Value>
    [[nodiscard]] static auto processForEach(Function &function, Value &&value) -> util::LoopStatus;

    [[nodiscard]] auto rangeSpan() const noexcept -> std::span<const CharRange>;
    [[nodiscard]] static auto inlineRangeCount(const InlineRanges &ranges) noexcept -> std::size_t;
    void addWithCapacity(CharRange range, std::size_t capacityHint);
    void ensureSharedCapacity(std::size_t requiredCapacity);
    [[nodiscard]] static auto nextScalar(Char character) noexcept -> std::optional<Char>;
    [[nodiscard]] static auto previousScalar(Char character) noexcept -> std::optional<Char>;

private:
    Storage _storage{InlineRanges{}}; ///< The inline or shared normalized range storage.
};

}

#include "CharSet.tpp"
