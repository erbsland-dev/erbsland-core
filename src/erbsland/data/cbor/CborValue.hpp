// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CborFormatOptions.hpp"
#include "CborLink.hpp"
#include "CborParseOptions.hpp"
#include "CborType.hpp"
#include "CborValue_fwd.hpp"

#include "impl/CborValueData_fwd.hpp"

#include "../../mem/ByteBlock.hpp"
#include "../../text/String.hpp"
#include "../../text/StringMap.hpp"
#include "../../time/DateTime.hpp"
#include "../../unit/ItemIndex.hpp"
#include "../../util/List.hpp"

#include <concepts>
#include <cstdint>
#include <memory>
#include <optional>
#include <type_traits>

namespace erbsland::data::cbor {
/// A copy-on-write CBOR value tree.
/// @seedoc{/reference/data/cbor}
/// @tested{CborValueTest}
class CborValue final {
public:
    // defaults
    CborValue() noexcept = default;

public: // value constructors
    /// Create a CBOR Boolean value.
    /// @param value The Boolean to store.
    CborValue(bool value); // NOLINT(*-explicit-constructor)
    /// Create a CBOR signed 64-bit integer value.
    /// @param value The signed 64-bit integer to store.
    CborValue(int64_t value); // NOLINT(*-explicit-constructor)
    /// Create a CBOR unsigned 64-bit integer value.
    /// @param value The unsigned 64-bit integer to store.
    CborValue(uint64_t value); // NOLINT(*-explicit-constructor)
    /// Create a CBOR double-precision number value.
    /// @param value The double-precision number to store.
    CborValue(double value); // NOLINT(*-explicit-constructor)
    /// Create a CBOR text string value.
    /// @param value The text string to store.
    CborValue(text::String value); // NOLINT(*-explicit-constructor)
    /// Create a CBOR binary byte string value.
    /// @param value The binary byte string to store.
    CborValue(mem::ByteBlock value); // NOLINT(*-explicit-constructor)
    /// Create a CBOR array value.
    /// @param value The array to store.
    CborValue(CborArray value); // NOLINT(*-explicit-constructor)
    /// Create a CBOR string-keyed map value.
    /// @param value The string-keyed map to store.
    CborValue(CborMap value); // NOLINT(*-explicit-constructor)
    /// Create a CBOR UTC date/time value.
    /// @param value The UTC date/time to store.
    CborValue(time::DateTime value); // NOLINT(*-explicit-constructor)
    /// Create a CBOR CID link value.
    /// @param value The CID link to store.
    CborValue(CborLink value); // NOLINT(*-explicit-constructor)
    /// Create an integer from any smaller native width.
    template <std::integral T>
        requires(
            !std::same_as<std::remove_cv_t<T>, bool> && !std::same_as<std::remove_cv_t<T>, int64_t> &&
            !std::same_as<std::remove_cv_t<T>, uint64_t>)
    /// Create a CBOR value value.
    /// @param value The value to store.
    CborValue(T value) : CborValue{static_cast<std::conditional_t<std::is_signed_v<T>, int64_t, uint64_t>>(value)} {}

public: // defaults
    ~CborValue() = default;
    CborValue(const CborValue &) noexcept = default;
    CborValue(CborValue &&) noexcept = default;
    auto operator=(const CborValue &) noexcept -> CborValue & = default;
    auto operator=(CborValue &&) noexcept -> CborValue & = default;

public: // accessors
    /// Get the semantic type.
    [[nodiscard]] auto type() const noexcept -> CborType;
    /// Test for a semantic type.
    [[nodiscard]] auto is(CborType expected) const noexcept -> bool { return type() == expected; }
    /// Get the array or map size, or zero.
    [[nodiscard]] auto itemCount() const noexcept -> unit::ItemCount;
    /// Get a child, or null when absent.
    [[nodiscard]] auto get(unit::ItemIndex index) const -> CborValue;
    /// Get a child, or null when absent.
    [[nodiscard]] auto get(const text::String &key) const -> CborValue;
    /// Get a child or throw for an invalid index or type.
    [[nodiscard]] auto getOrThrow(unit::ItemIndex index) const -> CborValue;
    /// Get a child or throw for a missing key or invalid type.
    [[nodiscard]] auto getOrThrow(const text::String &key) const -> CborValue;
    /// Get a Boolean value.
    [[nodiscard]] auto getBool() const noexcept -> std::optional<bool>;
    /// Get a signed integer.
    [[nodiscard]] auto getSigned() const noexcept -> std::optional<int64_t>;
    /// Get an unsigned integer.
    [[nodiscard]] auto getUnsigned() const noexcept -> std::optional<uint64_t>;
    /// Get a floating-point value.
    [[nodiscard]] auto getFloat() const noexcept -> std::optional<double>;
    /// Get a text value.
    [[nodiscard]] auto getText() const noexcept -> std::optional<text::String>;
    /// Get a byte-string value.
    [[nodiscard]] auto getBytes() const noexcept -> std::optional<mem::ByteBlock>;
    /// Get a CID link's bytes.
    [[nodiscard]] auto getLinkBytes() const noexcept -> std::optional<mem::ByteBlock>;
    /// Get a tagged date/time.
    [[nodiscard]] auto getDateTime() const noexcept -> std::optional<time::DateTime>;
    /// Get a copy of an array.
    [[nodiscard]] auto getArray() const noexcept -> std::optional<CborArray>;
    /// Get a copy of a map.
    [[nodiscard]] auto getMap() const noexcept -> std::optional<CborMap>;

public: // mutation
    /// Replace or append an array child.
    auto set(unit::ItemIndex index, CborValue value) -> CborValue &;
    /// Replace or add a map member.
    auto set(const text::String &key, CborValue value) -> CborValue &;
    /// Append an array child.
    auto append(CborValue value) -> CborValue &;

public: // conversion
    /// Serialize one CBOR data item.
    [[nodiscard]] auto toByteBlock(CborFormatOptions options = {}) const -> mem::ByteBlock;
    /// Parse one complete CBOR data item, returning no value on error.
    [[nodiscard]] static auto fromByteBlock(const mem::ByteBlock &bytes, CborParseOptions options = {}) noexcept
        -> std::optional<CborValue>;
    /// Parse one complete CBOR data item.
    /// @throws err::ParseError For malformed or unsupported input.
    [[nodiscard]] static auto fromByteBlockOrThrow(const mem::ByteBlock &bytes, CborParseOptions options = {})
        -> CborValue;

private: // mutation
    /// Copy shared storage before mutation.
    void detach();

private:                                        // data
    std::shared_ptr<impl::CborValueData> _data; ///< Shared value data; null denotes CBOR null.
};
}
