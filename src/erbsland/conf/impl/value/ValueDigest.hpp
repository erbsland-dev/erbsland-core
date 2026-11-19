// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Value_fwd.hpp"
#include "ValueDigest_fwd.hpp"

#include "../../../cryptology/Hasher.hpp"
#include "../../../mem/ByteBlock.hpp"
#include "../../../mem/ByteSpan.hpp"
#include "../../../re/RegEx_fwd.hpp"
#include "../../../text/String.hpp"
#include "../../../time/CalendarDelta.hpp"
#include "../../../time/Date.hpp"
#include "../../../time/DateTime.hpp"
#include "../../../time/Time.hpp"
#include "../../../time/TimeWithZone.hpp"
#include "../../../time/TimeZone.hpp"
#include "../../Document_fwd.hpp"
#include "../../Float.hpp"
#include "../../Integer.hpp"
#include "../../Name.hpp"
#include "../../Value_fwd.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <type_traits>

namespace erbsland::conf::impl {

/// Stream a framed configuration value tree directly into a hasher.
/// @tested{ValueDigestTest}
class ValueDigest final {
private:
    /// One active ancestor in the iterative depth-first traversal.
    struct Frame {
        std::span<const ValuePtr> children; ///< The ordered direct children.
        std::size_t next{0};                ///< The next child to visit.
    };

public:
    /// Begin a digest stream using an explicit fixed-output hash algorithm.
    /// @param algorithm The hash algorithm.
    explicit ValueDigest(cryptology::HashAlgorithm algorithm) : _hasher{algorithm} {}

public: // tree traversal
    /// Digest an ordinary value and its descendants.
    /// @param root The selected root, whose own name participates.
    /// @return The completed tree digest.
    [[nodiscard]] auto calculate(const Value &root) -> mem::ByteBlock;
    /// Digest a document and its stored children.
    /// @param root The document root.
    /// @param children Its direct children in configuration order.
    /// @return The completed tree digest.
    [[nodiscard]] auto calculate(const conf::Document &root, std::span<const ValuePtr> children) -> mem::ByteBlock;

public: // scalar payloads
    /// Add an integer's stored native bits.
    /// @param value The initialized scalar value.
    void update(const Integer &value) { native(value); }
    /// Add a boolean's logical value as one initialized byte.
    /// @param value The stored boolean.
    void update(bool value) { native(static_cast<uint8_t>(value)); }
    /// Add a floating-point value's stored native bits without normalization.
    /// @param value The initialized floating-point value.
    void update(const Float &value) { native(value); }
    /// Add a length-delimited UTF-8 string using its stored bytes.
    /// @param value The stored text.
    void update(const text::String &value);
    /// Add a length-delimited binary value without copying its bytes.
    /// @param value The stored byte block.
    void update(const mem::ByteBlock &value);
    /// Add a date without formatting it.
    /// @param value The stored date.
    void update(const time::Date &value);
    /// Add a floating time, preserving its storage kind.
    /// @param value The stored time.
    void update(const time::Time &value);
    /// Add a time and its explicit zone, preserving its storage kind.
    /// @param value The stored zoned time.
    void update(const time::TimeWithZone &value);
    /// Add every stored date-time component, including its offset and zone.
    /// @param value The stored date-time.
    void update(const time::DateTime &value);
    /// Add each calendar-delta component separately, excluding object padding.
    /// @param value The stored calendar delta.
    void update(const time::CalendarDelta &value);
    /// Add the pattern bytes and initial flags without compiling the expression.
    /// @param value The stored non-null regular expression.
    void update(const re::RegExPtr &value);

private:
    /// Hash one fully initialized arithmetic or enumeration object, never an aggregate layout.
    template <typename T>
        requires(std::is_arithmetic_v<T> || std::is_enum_v<T>)
    void native(const T &value) {
        _hasher.update(mem::toConstByteSpan(std::as_bytes(std::span<const T>{&value, 1})));
    }
    /// Add one framed byte sequence.
    /// @param value The byte sequence to add.
    void bytes(mem::ConstByteSpan value);
    /// Add the identity, type, and child count of a node.
    /// @param value The node whose own identity is included.
    /// @param childCount Its number of direct children.
    void nodeHeader(const conf::Value &value, std::size_t childCount);
    /// Add a time's numeric fields without a storage-kind tag.
    /// @param value The stored time.
    void timeParts(const time::Time &value);
    /// Add zone identity without its native object layout.
    /// @param value The stored zone.
    void timeZone(const time::TimeZone &value);
    /// Add every descendant in configuration order, retaining only active ancestors.
    /// @param children The first level of ordered descendants.
    void descendants(std::span<const ValuePtr> children);

private:
    cryptology::Hasher _hasher; ///< The only hasher used for the complete tree.
};

}
