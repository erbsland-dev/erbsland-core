// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Random_fwd.hpp"

#include "../math/IntegerMath.hpp"
#include "../math/IntegerRange.hpp"
#include "../math/IntegerTypes.hpp"
#include "../mem/ByteBlock.hpp"
#include "../mem/ByteBuffer.hpp"
#include "../text/CharSet.hpp"
#include "../text/StringEditor.hpp"
#include "../unit/ByteLength.hpp"
#include "../unit/CpLength.hpp"
#include "../unit/ItemCount.hpp"
#include "../unit/ItemIndex.hpp"
#include "../util/HashSet.hpp"
#include "../util/List.hpp"
#include "../util/Set.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <span>
#include <type_traits>
#include <vector>

namespace erbsland::random {

/// The common interface for random number generators.
/// @seedoc{/topics/random/overview}
/// @tested{RandomTest}
class Random {
public:
    // defaults/deletions
    /// Destroy this random generator.
    virtual ~Random() = default;

    Random(const Random &) = delete;
    auto operator=(const Random &) -> Random & = delete;

public:
    /// Create a random native integer in the inclusive range.
    /// @tparam T The native integer result type.
    /// @param minimum The lower inclusive bound. Reversed bounds are ordered automatically.
    /// @param maximum The upper inclusive bound. Reversed bounds are ordered automatically.
    /// @return A random integer in the inclusive range `[minimum, maximum]`.
    template <math::NativeInteger T>
    [[nodiscard]] auto selectInteger(T minimum, T maximum) -> T;
    /// Create a random native integer in the inclusive range.
    /// @tparam T The native integer result type.
    /// @param range The inclusive integer range.
    /// @return A random integer in the inclusive range.
    template <math::NativeInteger T>
    [[nodiscard]] auto selectInteger(math::IntegerRange<T> range) -> T;
    /// Create a list of random native integers.
    /// @tparam T The native integer result type.
    /// @param count The number of values to build. Zero or infinite counts return an empty list.
    /// @param minimum The lower inclusive bound. Reversed bounds are ordered automatically.
    /// @param maximum The upper inclusive bound. Reversed bounds are ordered automatically.
    /// @return A list with `count` random integers, or an empty list.
    template <math::NativeInteger T>
    [[nodiscard]] auto buildIntegerList(unit::ItemCount count, T minimum, T maximum) -> util::List<T>;
    /// Create a random UTF-8 string with characters from the given set.
    /// @param length The number of Unicode code points to build. Zero or infinite lengths return an empty string.
    /// @param characters The character choices. An empty set returns an empty string.
    /// @return A random UTF-8 string, or an empty string.
    /// @throws err::OutOfRangeError If the length exceeds the maximum possible value.
    [[nodiscard]] auto buildString(unit::CpLength length, const text::CharSet &characters) -> text::String;
    /// Create a block of random bytes.
    /// @param length The number of bytes to build. Zero or infinite lengths return an empty block.
    /// @return A block with random bytes, or an empty block.
    [[nodiscard]] auto buildByteBlock(unit::ByteLength length) -> mem::ByteBlock;
    /// Create a deep-copying buffer of random bytes.
    /// @param length The number of bytes to build. Zero or infinite lengths return an empty buffer.
    /// Secure generators return a buffer with sensitive mode enabled.
    /// @return A buffer with random bytes, or an empty buffer.
    [[nodiscard]] auto buildByteBuffer(unit::ByteLength length) -> mem::ByteBuffer;
    /// Select a valid element index for a container with `count` elements.
    /// @param count The number of available elements.
    /// @return A random index in `[0, count)`, or `ItemIndex::noIndex()` for zero or infinite counts.
    [[nodiscard]] auto selectIndex(unit::ItemCount count) -> unit::ItemIndex;

public: // element selection
    /// Select one random element from the given choices.
    /// @tparam T The element type.
    /// @param choices The available choices. Must not be empty unless `valueIfEmpty` shall be returned.
    /// @param valueIfEmpty The fallback value to return when `choices` is empty.
    /// @return A selected element, or `valueIfEmpty` for empty choices.
    template <typename T>
    [[nodiscard]] auto selectElement(std::span<const T> choices, const T &valueIfEmpty = {}) -> T;
    /// Select one random element from the given choices.
    /// @tparam T The element type.
    /// @param choices The available choices. Must not be empty unless `valueIfEmpty` shall be returned.
    /// @param valueIfEmpty The fallback value to return when `choices` is empty.
    /// @return A selected element, or `valueIfEmpty` for empty choices.
    template <typename T>
    [[nodiscard]] auto selectElement(const std::vector<T> &choices, const T &valueIfEmpty = {}) -> T;
    /// Select one random element from the given choices.
    /// @tparam T The element type.
    /// @param choices The available choices. Must not be empty unless `valueIfEmpty` shall be returned.
    /// @param valueIfEmpty The fallback value to return when `choices` is empty.
    /// @return A selected element, or `valueIfEmpty` for empty choices.
    template <typename T>
    [[nodiscard]] auto selectElement(std::initializer_list<T> choices, const T &valueIfEmpty = {}) -> T;
    /// Select one random element from the given choices.
    /// @tparam T The element type.
    /// @tparam Self The list CRTP type.
    /// @param choices The available choices. Must not be empty unless `valueIfEmpty` shall be returned.
    /// @param valueIfEmpty The fallback value to return when `choices` is empty.
    /// @return A selected element, or `valueIfEmpty` for empty choices.
    template <typename T, typename Self>
    [[nodiscard]] auto selectElement(const util::List<T, Self> &choices, const T &valueIfEmpty = {}) -> T;
    /// Select one random element from the given choices.
    /// @tparam T The element type.
    /// @tparam Compare The ordered-set comparison type.
    /// @tparam Self The ordered-set CRTP type.
    /// @param choices The available choices. Must not be empty unless `valueIfEmpty` shall be returned.
    /// @param valueIfEmpty The fallback value to return when `choices` is empty.
    /// @return A selected element, or `valueIfEmpty` for empty choices.
    template <typename T, typename Compare, typename Self>
    [[nodiscard]] auto selectElement(const util::Set<T, Compare, Self> &choices, const T &valueIfEmpty = {}) -> T;
    /// Select one random element from the given choices.
    /// @tparam T The element type.
    /// @tparam Hash The hash-set hash type.
    /// @tparam Equal The hash-set equality type.
    /// @tparam Self The hash-set CRTP type.
    /// @param choices The available choices. Must not be empty unless `valueIfEmpty` shall be returned.
    /// @param valueIfEmpty The fallback value to return when `choices` is empty.
    /// @return A selected element, or `valueIfEmpty` for empty choices.
    template <typename T, typename Hash, typename Equal, typename Self>
    [[nodiscard]] auto selectElement(const util::HashSet<T, Hash, Equal, Self> &choices, const T &valueIfEmpty = {})
        -> T;
    /// Build a list by sampling elements with replacement.
    /// @tparam T The element type.
    /// @param count The number of elements to build. Zero or infinite counts return an empty list.
    /// @param choices The choices to sample from. Empty choices return an empty list.
    /// @return A list with sampled elements, or an empty list.
    template <typename T>
    [[nodiscard]] auto buildElementList(unit::ItemCount count, std::span<const T> choices) -> util::List<T>;
    /// Build a list by sampling elements with replacement.
    /// @tparam T The element type.
    /// @tparam Self The list CRTP type.
    /// @param count The number of elements to build. Zero or infinite counts return an empty list.
    /// @param choices The choices to sample from. Empty choices return an empty list.
    /// @return A matching list with sampled elements, or an empty list.
    template <typename T, typename Self>
    [[nodiscard]] auto buildElementList(unit::ItemCount count, const util::List<T, Self> &choices) ->
        typename util::List<T, Self>::Self;
    /// Build a list by sampling elements with replacement.
    /// @tparam T The element type.
    /// @tparam Compare The ordered-set comparison type.
    /// @tparam Self The ordered-set CRTP type.
    /// @param count The number of elements to build. Zero or infinite counts return an empty list.
    /// @param choices The choices to sample from. Empty choices return an empty list.
    /// @return A list with sampled elements, or an empty list.
    template <typename T, typename Compare, typename Self>
    [[nodiscard]] auto buildElementList(unit::ItemCount count, const util::Set<T, Compare, Self> &choices)
        -> util::List<T>;
    /// Build a list by sampling elements with replacement.
    /// @tparam T The element type.
    /// @tparam Hash The hash-set hash type.
    /// @tparam Equal The hash-set equality type.
    /// @tparam Self The hash-set CRTP type.
    /// @param count The number of elements to build. Zero or infinite counts return an empty list.
    /// @param choices The choices to sample from. Empty choices return an empty list.
    /// @return A list with sampled elements, or an empty list.
    template <typename T, typename Hash, typename Equal, typename Self>
    [[nodiscard]] auto buildElementList(unit::ItemCount count, const util::HashSet<T, Hash, Equal, Self> &choices)
        -> util::List<T>;
    /// Build a list by sampling elements without replacement.
    /// @tparam T The element type.
    /// @param count The number of elements to build. Zero or infinite counts return an empty list.
    /// @param choices The choices to sample from. Empty choices return an empty list.
    /// @return A list with unique sampled elements. The list is capped to the number of choices.
    template <typename T>
    [[nodiscard]] auto buildUniqueElementList(unit::ItemCount count, std::span<const T> choices) -> util::List<T>;
    /// Build a list by sampling elements without replacement.
    /// @tparam T The element type.
    /// @tparam Self The list CRTP type.
    /// @param count The number of elements to build. Zero or infinite counts return an empty list.
    /// @param choices The choices to sample from. Empty choices return an empty list.
    /// @return A matching list with unique sampled elements. The list is capped to the number of choices.
    template <typename T, typename Self>
    [[nodiscard]] auto buildUniqueElementList(unit::ItemCount count, const util::List<T, Self> &choices) ->
        typename util::List<T, Self>::Self;
    /// Build a list by sampling elements without replacement.
    /// @tparam T The element type.
    /// @tparam Compare The ordered-set comparison type.
    /// @tparam Self The ordered-set CRTP type.
    /// @param count The number of elements to build. Zero or infinite counts return an empty list.
    /// @param choices The choices to sample from. Empty choices return an empty list.
    /// @return A list with unique sampled elements. The list is capped to the number of choices.
    template <typename T, typename Compare, typename Self>
    [[nodiscard]] auto buildUniqueElementList(unit::ItemCount count, const util::Set<T, Compare, Self> &choices)
        -> util::List<T>;
    /// Build a list by sampling elements without replacement.
    /// @tparam T The element type.
    /// @tparam Hash The hash-set hash type.
    /// @tparam Equal The hash-set equality type.
    /// @tparam Self The hash-set CRTP type.
    /// @param count The number of elements to build. Zero or infinite counts return an empty list.
    /// @param choices The choices to sample from. Empty choices return an empty list.
    /// @return A list with unique sampled elements. The list is capped to the number of choices.
    template <typename T, typename Hash, typename Equal, typename Self>
    [[nodiscard]] auto buildUniqueElementList(unit::ItemCount count, const util::HashSet<T, Hash, Equal, Self> &choices)
        -> util::List<T>;

public: // shuffling
    /// Shuffle a random-access span in place.
    /// @tparam T The element type.
    /// @param values The values to shuffle. Empty and single-element spans are unchanged.
    template <typename T>
    void shuffle(std::span<T> values);
    /// Shuffle a vector in place.
    /// @tparam T The element type.
    /// @param values The values to shuffle. Empty and single-element vectors are unchanged.
    template <typename T>
    void shuffle(std::vector<T> &values);
    /// Shuffle a list in place.
    /// @tparam T The element type.
    /// @tparam Self The list CRTP type.
    /// @param values The values to shuffle. Empty and single-element lists are unchanged.
    template <typename T, typename Self>
    void shuffle(util::List<T, Self> &values);

public: // source methods
    /// Test if this generator is suitable for security-sensitive random data.
    /// Secure generators automatically mark byte blocks created by `buildByteBlock()` as sensitive.
    [[nodiscard]] virtual auto isSecure() const noexcept -> bool { return false; }
    /// Generate a random 32-bit signed integer in the inclusive range.
    /// @param minimum The lower inclusive bound. Reversed bounds are ordered automatically.
    /// @param maximum The upper inclusive bound. Reversed bounds are ordered automatically.
    /// @return A random integer in the inclusive range `[minimum, maximum]`.
    [[nodiscard]] virtual auto getInt32(int32_t minimum, int32_t maximum) -> int32_t = 0;
    /// Generate a random 32-bit unsigned integer in the inclusive range.
    /// @param minimum The lower inclusive bound. Reversed bounds are ordered automatically.
    /// @param maximum The upper inclusive bound. Reversed bounds are ordered automatically.
    /// @return A random integer in the inclusive range `[minimum, maximum]`.
    [[nodiscard]] virtual auto getUInt32(uint32_t minimum, uint32_t maximum) -> uint32_t = 0;
    /// Generate a random 64-bit signed integer in the inclusive range.
    /// @param minimum The lower inclusive bound. Reversed bounds are ordered automatically.
    /// @param maximum The upper inclusive bound. Reversed bounds are ordered automatically.
    /// @return A random integer in the inclusive range `[minimum, maximum]`.
    [[nodiscard]] virtual auto getInt64(int64_t minimum, int64_t maximum) -> int64_t = 0;
    /// Generate a random 64-bit unsigned integer in the inclusive range.
    /// @param minimum The lower inclusive bound. Reversed bounds are ordered automatically.
    /// @param maximum The upper inclusive bound. Reversed bounds are ordered automatically.
    /// @return A random integer in the inclusive range `[minimum, maximum]`.
    [[nodiscard]] virtual auto getUInt64(uint64_t minimum, uint64_t maximum) -> uint64_t = 0;
    /// Generate a random floating-point value.
    /// @param minimum The lower bound. Reversed bounds are ordered automatically.
    /// @param maximum The upper bound. Reversed bounds are ordered automatically.
    /// @return A random floating-point value in the range supported by the generator.
    [[nodiscard]] virtual auto getDouble(double minimum, double maximum) -> double = 0;
    /// Generate a random boolean value.
    /// @return A random boolean value.
    [[nodiscard]] virtual auto getBool() -> bool = 0;
    /// Fill the destination with random bytes.
    /// @param destination The byte span to fill. An empty span is accepted.
    virtual void fillBytes(std::span<std::byte> destination) = 0;

protected:
    /// Create a random generator base.
    Random() = default;
};

}

#include "Random.tpp"
