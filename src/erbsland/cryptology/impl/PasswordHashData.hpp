// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "PasswordHashData_fwd.hpp"

#include "../PasswordHashKey.hpp"
#include "../PasswordHashPolicy.hpp"

#include "../../mem/ByteBlock.hpp"
#include "../../mem/ByteSpan.hpp"
#include "../../text/base_n/BaseNFormat_fwd.hpp"
#include "../../text/impl/NamedKeyEntry_fwd.hpp"
#include "../../text/impl/NamedKeyFormat_fwd.hpp"
#include "../../text/String.hpp"
#include "../../text/StringEditor_fwd.hpp"
#include "../../text/StringLiteral_fwd.hpp"
#include "../../util/List_fwd.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>

namespace erbsland::cryptology::impl {

/// Immutable canonical record data behind a `PasswordHash`.
/// @tested{PasswordHasherTest}
class PasswordHashData final {
private:
    /// The known record fields in canonical storage order.
    enum class Field : int {
        Format,          ///< The record format identifier.
        Version,         ///< The record format version.
        Algorithm,       ///< The password-hashing algorithm.
        ArgonVersion,    ///< The Argon2 version.
        Memory,          ///< The Argon2 memory cost.
        Passes,          ///< The Argon2 pass count.
        Parallelization, ///< The algorithm parallelization parameter.
        Cost,            ///< The scrypt cost parameter.
        BlockSize,       ///< The scrypt block-size parameter.
        Mode,            ///< The verifier protection mode.
        KeyIdentifier,   ///< The optional key identifier.
        Salt,            ///< The public salt.
        Data,            ///< The protected verifier.
    };
    /// A list of fields.
    using FieldList = util::List<text::impl::NamedKeyEntry>;

public:
    /// Test whether the record uses an application key.
    [[nodiscard]] auto isKeyed() const noexcept -> bool { return _keyed; }
    /// Test whether the record must be replaced for the active policy and key.
    /// @param policy The active password-hashing policy.
    /// @param activeKey The active application key, or null for explicitly unkeyed operation.
    /// @return `true` if the record does not match the active configuration.
    [[nodiscard]] auto needsReplacement(
        const PasswordHashPolicy &policy, const PasswordHashKey *activeKey) const noexcept -> bool;
    /// Test a raw password derivation against the stored verifier.
    /// @param rawVerifier The raw output from the record's password-hashing algorithm.
    /// @param key The matching application key, or null for an unkeyed record.
    /// @return `true` if the protected verifier matches in constant time.
    [[nodiscard]] auto matchesVerifier(mem::ConstByteSpan rawVerifier, const PasswordHashKey *key) const -> bool;
    /// Perform verifier protection and a constant-time comparison for an invalid record.
    /// This mirrors the verification work after derivation without allocating or serializing a throwaway record.
    /// @param policy The active password-hashing policy.
    /// @param key The active application key, or null for explicitly unkeyed operation.
    /// @param salt The dummy public salt.
    /// @param rawVerifier The dummy raw derivation.
    static void performDummyVerification(
        const PasswordHashPolicy &policy,
        const PasswordHashKey *key,
        mem::ConstByteSpan salt,
        mem::ConstByteSpan rawVerifier);

public: // accessors
    /// Get the parsed password-hashing policy.
    [[nodiscard]] auto policy() const noexcept -> const PasswordHashPolicy & { return _policy; }
    /// Get the public key identifier, if present.
    [[nodiscard]] auto keyIdentifier() const noexcept -> const std::optional<text::String> & { return _keyIdentifier; }
    /// Get the public salt.
    [[nodiscard]] auto salt() const noexcept -> const mem::ByteBlock & { return _salt; }

public: // conversion
    /// Return the canonical storage representation.
    [[nodiscard]] auto toString() const noexcept -> const text::String & { return _canonical; }
    /// Parse and validate a canonical password-hash record.
    /// @param text The storage record.
    /// @return The immutable parsed record data.
    /// @throws err::ParseError If the record is malformed, noncanonical, unsupported, or exceeds resource limits.
    [[nodiscard]] static auto fromStringOrThrow(const text::String &text) -> PasswordHashDataPtr;

public: // factory
    /// Create a canonical record from a raw password derivation.
    /// @param policy The password-hashing policy used for the derivation.
    /// @param key The application key, or null for an explicitly unkeyed record.
    /// @param salt The public salt.
    /// @param rawVerifier The raw output from the password-hashing algorithm.
    /// @return The immutable canonical record data.
    /// @throws err::LogicError If salt or verifier lengths violate the policy invariant.
    [[nodiscard]] static auto create(
        PasswordHashPolicy policy, const PasswordHashKey *key, mem::ConstByteSpan salt, mem::ConstByteSpan rawVerifier)
        -> PasswordHashDataPtr;

private:
    /// Create immutable password-hash record data from validated fields.
    PasswordHashData(
        PasswordHashPolicy policy,
        bool keyed,
        std::optional<text::String> keyIdentifier,
        mem::ByteBlock salt,
        mem::ByteBlock verifier,
        text::String headerThroughSalt,
        text::String canonical) noexcept;
    /// Get the Base64 format used for storage fields.
    [[nodiscard]] static auto storageBase64Format() -> text::base_n::BaseNFormat;
    /// Get the named-key format for password-hash records.
    [[nodiscard]] static auto passwordHashFormat() -> const text::impl::NamedKeyFormat &;
    /// Get a field value while validating its key and position.
    [[nodiscard]] static auto fieldValue(const FieldList &fields, unit::ItemIndex index, Field expected)
        -> const text::String &;
    /// Parse a canonical bounded unsigned integer.
    [[nodiscard]] static auto parseCanonicalInteger(const text::String &text, uint64_t maximum) -> uint64_t;
    /// Encode bytes for a canonical storage field.
    [[nodiscard]] static auto encodeBytes(mem::ConstByteSpan bytes) -> text::String;
    /// Decode bytes from a canonical storage field.
    [[nodiscard]] static auto decodeBytes(const text::String &text, unit::ByteLength expectedLength) -> mem::ByteBlock;
    /// Append a named field to a canonical record.
    static void appendField(text::StringEditor &result, const text::StringLiteral &key, const text::String &value);
    /// Build the canonical record header through the salt field.
    [[nodiscard]] static auto buildHeader(
        const PasswordHashPolicy &policy,
        bool keyed,
        const std::optional<text::String> &keyIdentifier,
        mem::ConstByteSpan salt) -> text::String;
    /// Build a complete canonical record from its header and verifier.
    [[nodiscard]] static auto buildRecord(const text::String &header, mem::ConstByteSpan verifier) -> text::String;
    /// Protect a raw verifier using the record header and optional application key.
    [[nodiscard]] static auto protectVerifier(
        mem::ConstByteSpan rawVerifier, const text::String &header, const PasswordHashKey *key) -> mem::ByteBlock;

private:
    PasswordHashPolicy _policy;                 ///< The parsed algorithm and cost policy.
    bool _keyed{};                              ///< Whether the verifier is protected by an application key.
    std::optional<text::String> _keyIdentifier; ///< The optional public key-rotation identifier.
    mem::ByteBlock _salt;                       ///< The decoded public salt.
    mem::ByteBlock _verifier;                   ///< The decoded stored verifier.
    text::String _headerThroughSalt;            ///< The canonical authenticated header through the salt field.
    text::String _canonical;                    ///< The complete canonical storage record.
};

}
