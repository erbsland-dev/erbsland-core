// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../Asn1Limits.hpp"
#include "../Asn1Node.hpp"

#include "../../../mem/ByteBlockEditor.hpp"

#include <array>

namespace erbsland::data::asn1::impl {

/// Shared definite-length writer, including nested scopes and output budgets.
/// @tested{DerEncoderTest BerCodecTest}
class Asn1Writer final {
public:
    /// Child ordering required by the enclosing schema.
    enum class Order {
        Insertion, ///< Preserve insertion order.
        Tag,       ///< Require ascending tags.
        Encoding,  ///< Require ascending complete encodings.
    };
    /// An encoder-specific handle for closing an open value.
    class Scope final {
        friend class Asn1Writer;
        /// Bind a token to one opening in the given writer.
        Scope(const Asn1Writer *owner, uint64_t id) noexcept : _owner{owner}, _id{id} {}
        const Asn1Writer *_owner; ///< Owning encoder.
        uint64_t _id;             ///< Unique opening within the encoder.
    };
    /// Create a writer with the given budgets.
    /// @param limits Output budgets.
    explicit Asn1Writer(Asn1Limits limits = {}) noexcept : _limits{limits} {}

    // defaults/deletions
    Asn1Writer(const Asn1Writer &) = delete;
    auto operator=(const Asn1Writer &) -> Asn1Writer & = delete;
    /// Get the output budgets.
    [[nodiscard]] auto limits() const noexcept -> Asn1Limits { return _limits; }
    /// Mark output storage as sensitive.
    void markAsSensitive() noexcept;
    /// Get the complete output after closing all scopes.
    [[nodiscard]] auto encoded() const -> mem::ByteBlock;
    /// Open a value and reserve its length octet.
    [[nodiscard]] auto begin(
        Asn1Tag tag, bool constructed, Order order = Order::Insertion, std::optional<mem::Byte> prefix = {}) -> Scope;
    /// Close the most recent scope.
    void end(Scope scope);
    /// Discard the most recent scope and its contents.
    void cancel(Scope scope);
    /// Append one complete primitive, optionally prefixing its content with a byte.
    void primitive(Asn1Tag tag, mem::ConstByteSpan content, std::optional<mem::Byte> prefix = {});
    /// Encode a signed two's-complement value.
    void integer(Asn1Tag tag, int64_t value);
    /// Append one validated DER node without decoding it again.
    void appendDer(const Asn1Node &node);
    /// Encode an OID directly into the output allocation.
    void objectIdentifier(const Asn1ObjectIdentifier &oid);

private:
    /// Track one open value and the ordering of its children.
    struct Frame {
        uint64_t id{};                                ///< Handle generation.
        unit::ByteIndex start;                        ///< Identifier start.
        unit::ByteIndex lengthIndex{};                ///< Reserved length octet.
        unit::ItemCount nodesBefore;                  ///< Budget before opening.
        Asn1Tag tag{Asn1UniversalType::None};         ///< Value identifier.
        Order order{};                                ///< Ordering for direct children.
        std::optional<unit::ByteIndex> previous{};    ///< Beginning of previous child.
        Asn1Tag previousTag{Asn1UniversalType::None}; ///< Previous child identifier.
    };
    /// Check available output space before appending bytes.
    void requireCapacity(unit::ByteCount extra) const;
    /// Check the depth and node budgets before emitting a value.
    void requireNode(std::size_t depth, unit::ItemCount count) const;
    /// Charge every embedded node against the enclosing document budgets.
    void accountTree(const Asn1Node &node, std::size_t depth, unit::ItemCount &count) const;
    /// Validate and record a completed child in its parent.
    void finishChild(Asn1Tag tag, unit::ByteIndex start);
    /// Resolve a handle for the active scope.
    [[nodiscard]] auto top(Scope scope) -> Frame &;
    Asn1Limits _limits;             ///< Resource limits.
    mem::ByteBlockEditor _data;     ///< Shared final byte storage.
    std::array<Frame, 32U> _frames; ///< One entry per open scope.
    std::size_t _depth{};           ///< Number of open scopes.
    unit::ItemCount _nodes;         ///< Total emitted values.
    uint64_t _serial{};             ///< Last issued scope identity.
};

}
