// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "BsonBinary.hpp"
#include "BsonFormatOptions.hpp"
#include "BsonOpaqueValue.hpp"
#include "BsonParseOptions.hpp"
#include "BsonType.hpp"
#include "BsonValue_fwd.hpp"

#include "impl/BsonValueData_fwd.hpp"

#include "../../mem/ByteBlock.hpp"
#include "../../text/String.hpp"
#include "../../text/StringMap.hpp"
#include "../../time/Timestamp.hpp"
#include "../../unit/ItemIndex.hpp"
#include "../../util/List.hpp"

#include <cstdint>
#include <memory>
#include <optional>

namespace erbsland::data::bson {
/// A copy-on-write BSON value tree.
/// @seedoc{/reference/data/bson}
/// @tested{BsonValueTest}
class BsonValue final {
public:
    // defaults
    BsonValue() noexcept = default;

public: // value constructors
    /// Create a BSON Boolean value.
    /// @param value The Boolean to store.
    BsonValue(bool value); // NOLINT(*-explicit-constructor)
    /// Create a BSON signed 32-bit integer value.
    /// @param value The signed 32-bit integer to store.
    BsonValue(int32_t value); // NOLINT(*-explicit-constructor)
    /// Create a BSON signed 64-bit integer value.
    /// @param value The signed 64-bit integer to store.
    BsonValue(int64_t value); // NOLINT(*-explicit-constructor)
    /// Create a BSON signed 8-bit integer value.
    /// @param value The signed 8-bit integer to store.
    BsonValue(int8_t value); // NOLINT(*-explicit-constructor)
    /// Create a BSON unsigned 8-bit integer value.
    /// @param value The unsigned 8-bit integer to store.
    BsonValue(uint8_t value); // NOLINT(*-explicit-constructor)
    /// Create a BSON signed 16-bit integer value.
    /// @param value The signed 16-bit integer to store.
    BsonValue(int16_t value); // NOLINT(*-explicit-constructor)
    /// Create a BSON unsigned 16-bit integer value.
    /// @param value The unsigned 16-bit integer to store.
    BsonValue(uint16_t value); // NOLINT(*-explicit-constructor)
    /// Create a BSON unsigned 32-bit integer value.
    /// @param value The unsigned 32-bit integer to store.
    BsonValue(uint32_t value); // NOLINT(*-explicit-constructor)
    /// Create a BSON unsigned 64-bit integer value.
    /// @param value The unsigned 64-bit integer to store.
    /// @throws err::ParameterError If the integer exceeds the BSON signed range.
    BsonValue(uint64_t value); // NOLINT(*-explicit-constructor)
    /// Create a BSON double-precision number value.
    /// @param value The double-precision number to store.
    BsonValue(double value); // NOLINT(*-explicit-constructor)
    /// Create a BSON text string value.
    /// @param value The text string to store.
    BsonValue(text::String value); // NOLINT(*-explicit-constructor)
    /// Create a BSON binary value with subtype value.
    /// @param value The binary value with subtype to store.
    BsonValue(BsonBinary value); // NOLINT(*-explicit-constructor)
    /// Create a BSON binary byte string value.
    /// @param value The binary byte string to store.
    BsonValue(mem::ByteBlock value); // NOLINT(*-explicit-constructor)
    /// Create a BSON UTC date/time value.
    /// @param value The UTC date/time to store.
    BsonValue(time::Timestamp value); // NOLINT(*-explicit-constructor)
    /// Create a BSON array value.
    /// @param value The array to store.
    BsonValue(BsonArray value); // NOLINT(*-explicit-constructor)
    /// Create a BSON document value.
    /// @param value The document to store.
    BsonValue(BsonDocument value); // NOLINT(*-explicit-constructor)
    /// Create a BSON opaque wire value value.
    /// @param value The opaque wire value to store.
    BsonValue(BsonOpaqueValue value); // NOLINT(*-explicit-constructor)

public:                               // defaults
    ~BsonValue() = default;
    BsonValue(const BsonValue &) noexcept = default;
    BsonValue(BsonValue &&) noexcept = default;
    auto operator=(const BsonValue &) noexcept -> BsonValue & = default;
    auto operator=(BsonValue &&) noexcept -> BsonValue & = default;

public: // accessors
    /// Get the semantic type.
    [[nodiscard]] auto type() const noexcept -> BsonType;
    /// Test for a semantic type.
    [[nodiscard]] auto is(BsonType expected) const noexcept -> bool { return type() == expected; }
    /// Get the array or document size, or zero.
    [[nodiscard]] auto itemCount() const noexcept -> unit::ItemCount;
    /// Get a child or null when absent.
    [[nodiscard]] auto get(unit::ItemIndex index) const -> BsonValue;
    /// Get a child or null when absent.
    [[nodiscard]] auto get(const text::String &key) const -> BsonValue;
    /// Get a child or throw.
    [[nodiscard]] auto getOrThrow(unit::ItemIndex index) const -> BsonValue;
    /// Get a child or throw.
    [[nodiscard]] auto getOrThrow(const text::String &key) const -> BsonValue;
    /// Get a Boolean value.
    [[nodiscard]] auto getBool() const noexcept -> std::optional<bool>;
    /// Get an exact signed integer across both BSON integer widths.
    [[nodiscard]] auto getInteger() const noexcept -> std::optional<int64_t>;
    /// Get a floating-point value.
    [[nodiscard]] auto getDouble() const noexcept -> std::optional<double>;
    /// Get a text value.
    [[nodiscard]] auto getText() const noexcept -> std::optional<text::String>;
    /// Get a binary value and its subtype.
    [[nodiscard]] auto getBinary() const noexcept -> std::optional<BsonBinary>;
    /// Get a UTC millisecond date/time.
    [[nodiscard]] auto getTimestamp() const noexcept -> std::optional<time::Timestamp>;
    /// Get a copy of an array.
    [[nodiscard]] auto getArray() const noexcept -> std::optional<BsonArray>;
    /// Get a copy of a document.
    [[nodiscard]] auto getDocument() const noexcept -> std::optional<BsonDocument>;
    /// Get an opaque value.
    [[nodiscard]] auto getOpaque() const noexcept -> std::optional<BsonOpaqueValue>;

public: // mutation
    /// Replace or append an array child.
    auto set(unit::ItemIndex index, BsonValue value) -> BsonValue &;
    /// Replace or add a document field.
    auto set(const text::String &key, BsonValue value) -> BsonValue &;
    /// Append an array child.
    auto append(BsonValue value) -> BsonValue &;

public: // conversion
    /// Serialize a document root to BSON.
    /// @throws err::ParameterError If this is not a document or a value cannot be encoded.
    [[nodiscard]] auto toByteBlock(BsonFormatOptions options = {}) const -> mem::ByteBlock;
    /// Parse one complete BSON document, returning no value on failure.
    [[nodiscard]] static auto fromByteBlock(const mem::ByteBlock &bytes, BsonParseOptions options = {}) noexcept
        -> std::optional<BsonValue>;
    /// Parse one complete BSON document.
    /// @throws err::ParseError For malformed or unsupported wire data.
    [[nodiscard]] static auto fromByteBlockOrThrow(const mem::ByteBlock &bytes, BsonParseOptions options = {})
        -> BsonValue;

private: // mutation
    /// Copy shared storage before mutation.
    void detach();

private:                                        // data
    std::shared_ptr<impl::BsonValueData> _data; ///< Shared value data; null denotes BSON null.
};
}
