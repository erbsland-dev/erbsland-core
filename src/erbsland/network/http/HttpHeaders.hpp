// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "HttpField.hpp"
#include "HttpHeaderLimits.hpp"
#include "HttpMediaType.hpp"

#include "../../text/StringList.hpp"
#include "../../unit/ByteCount.hpp"
#include "../../unit/ItemCount.hpp"
#include "../../unit/ItemIndex.hpp"
#include "../../util/List.hpp"

#include <optional>
#include <utility>

namespace erbsland::network {

/// An ordered copy-on-write list of HTTP fields.
using HttpFieldList = util::List<HttpField>;

/// An ordered, bounded, copy-on-write HTTP header collection.
/// Repeated fields and exact field values are preserved. Captured limits are operational and are not part of equality.
/// @seedoc{/reference/network/http_protocol}
/// @tested{HttpHeadersTest}
class HttpHeaders final {
public:
    /// Create an empty collection with default limits.
    HttpHeaders() = default;
    /// Create an empty collection with captured limits.
    explicit HttpHeaders(HttpHeaderLimits limits) noexcept : _limits{limits} {}
    /// Create a validated collection from fields and captured limits.
    /// @param fields The validated fields. Individual fields and the aggregate collection must fit the captured
    /// `limits`.
    /// @param limits The header limits captured by this collection.
    explicit HttpHeaders(HttpFieldList fields, HttpHeaderLimits limits = {});

    // defaults
    ~HttpHeaders() = default;
    HttpHeaders(const HttpHeaders &) noexcept = default;
    HttpHeaders(HttpHeaders &&) noexcept = default;
    auto operator=(const HttpHeaders &) noexcept -> HttpHeaders & = default;
    auto operator=(HttpHeaders &&) noexcept -> HttpHeaders & = default;

public: // operators
    /// Compare ordered fields while ignoring captured limits.
    [[nodiscard]] auto operator==(const HttpHeaders &other) const noexcept -> bool { return _fields == other._fields; }

public: // accessors
    /// Get the captured resource limits.
    [[nodiscard]] constexpr auto limits() const noexcept -> HttpHeaderLimits { return _limits; }
    /// Get the field count.
    [[nodiscard]] auto fieldCount() const noexcept -> unit::ItemCount { return _fields.count(); }
    /// Get the serialized byte length using `name: value` plus CRLF for each field.
    [[nodiscard]] auto serializedLength() const noexcept -> unit::ByteCount;
    /// Get a copy-on-write copy of all ordered fields.
    [[nodiscard]] auto fields() const noexcept -> HttpFieldList { return _fields; }
    /// Get a field by index, or an invalid field if the index is outside the list.
    [[nodiscard]] auto field(unit::ItemIndex index) const -> HttpField { return _fields.get(index); }

public: // lookup
    /// Test whether at least one field has the given name.
    [[nodiscard]] auto hasField(const HttpFieldName &name) const noexcept -> bool;
    /// Test whether at least one recognized field has the given type.
    [[nodiscard]] auto hasField(HttpFieldType type) const noexcept -> bool;
    /// Test whether at least one field has the given valid text name.
    [[nodiscard]] auto hasField(const text::String &name) const noexcept -> bool;
    /// Get the first exact value for a field, or empty text if absent.
    [[nodiscard]] auto getFirst(const HttpFieldName &name) const noexcept -> text::String;
    /// Get the first exact value for a recognized field, or empty text if absent.
    [[nodiscard]] auto getFirst(HttpFieldType type) const noexcept -> text::String;
    /// Get the first exact value for a valid text field name, or empty text if absent.
    [[nodiscard]] auto getFirst(const text::String &name) const noexcept -> text::String;
    /// Get every exact value for a field in original order.
    [[nodiscard]] auto getAll(const HttpFieldName &name) const -> text::StringList;
    /// Get every exact value for a recognized field in original order.
    [[nodiscard]] auto getAll(HttpFieldType type) const -> text::StringList;
    /// Get every exact value for a valid text field name in original order.
    [[nodiscard]] auto getAll(const text::String &name) const -> text::StringList;

public: // modifiers
    /// Append a field.
    /// @param field The field to store. The resulting collection must fit the captured header limits.
    auto addField(HttpField field) -> HttpHeaders &;
    /// Append a recognized field.
    /// @param type A valid recognized HTTP field type.
    /// @param value The field value. Must be valid for an HTTP field and keep the resulting collection within the
    /// captured limits.
    auto addField(HttpFieldType type, text::String value) -> HttpHeaders &;
    /// Append a field from text.
    /// @param name The field name. Must be a non-empty ASCII token.
    /// @param value The field value. Must be valid for an HTTP field and keep the resulting collection within the
    /// captured limits.
    auto addField(text::String name, text::String value) -> HttpHeaders &;
    /// Replace all matching fields with one field at the first matching position, or append it.
    /// @param field The field to store. The resulting collection must fit the captured header limits.
    auto setField(HttpField field) -> HttpHeaders &;
    /// Replace a recognized field.
    /// @param type A valid recognized HTTP field type.
    /// @param value The field value. Must be valid for an HTTP field and keep the resulting collection within the
    /// captured limits.
    auto setField(HttpFieldType type, text::String value) -> HttpHeaders &;
    /// Replace a field from text.
    /// @param name The field name. Must be a non-empty ASCII token.
    /// @param value The field value. Must be valid for an HTTP field and keep the resulting collection within the
    /// captured limits.
    auto setField(text::String name, text::String value) -> HttpHeaders &;
    /// Remove all fields with a matching name and return the number removed.
    auto removeAllFields(const HttpFieldName &name) -> unit::ItemCount;
    /// Remove all fields with a recognized type and return the number removed.
    auto removeAllFields(HttpFieldType type) -> unit::ItemCount;
    /// Remove all fields with a valid text name and return the number removed.
    auto removeAllFields(const text::String &name) -> unit::ItemCount;

public: // convenience
    /// Parse the first Content-Type field, or return no value if absent or invalid.
    [[nodiscard]] auto contentType() const noexcept -> std::optional<HttpMediaType>;
    /// Set one canonical Content-Type field.
    /// @param mediaType The canonical media type. Must be valid and fit the captured header limits.
    auto setContentType(const HttpMediaType &mediaType) -> HttpHeaders &;

private:
    /// Test whether a field has a name.
    [[nodiscard]] static auto matches(const HttpField &field, const HttpFieldName &name) noexcept -> bool;
    /// Validate all captured limits for a candidate field list.
    void validate(const HttpFieldList &fields) const;

private:
    HttpFieldList _fields;    ///< Ordered COW field storage.
    HttpHeaderLimits _limits; ///< Captured resource limits.
};

}
