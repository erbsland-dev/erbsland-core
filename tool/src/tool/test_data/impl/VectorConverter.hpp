// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "VectorSource.hpp"

#include <erbsland/ByteBlock.hpp>
#include <erbsland/json/JsonValue.hpp>
#include <erbsland/render/Environment.hpp>
#include <erbsland/render/Value.hpp>
#include <erbsland/StringMap.hpp>

namespace tool::test_data::impl {

using namespace el::text::literals;

/// Convert pinned source formats into response fixtures without validating negative cryptology payloads.
class VectorConverter final {
    /// JSON array of Wycheproof key or parameter groups.
    constexpr static auto cTestGroupsKey = "testGroups"_el;
    /// JSON public-key object or encoded payload, as defined by the source schema.
    constexpr static auto cPublicKeyKey = "publicKey"_el;
    /// NIST affine public-key x coordinate.
    constexpr static auto cQxKey = "Qx"_el;
    /// NIST affine public-key y coordinate.
    constexpr static auto cQyKey = "Qy"_el;
    /// Response verification result field.
    constexpr static auto cResultKey = "Result"_el;

public:
    /// Bind rendering and source attribution for one verified input.
    /// @param renderer Shared compiled template environment.
    /// @param source Supported source descriptor.
    /// @param revision Configured upstream revision.
    VectorConverter(el::render::EnvironmentPtr renderer, VectorSource source, el::String revision);

public: // conversion
    /// Convert a verified source and return complete relative-path/text outputs.
    /// @param data Exact verified source bytes.
    /// @return Prepared response fixtures.
    /// @throws el::ApplicationError If the source format or selected dimensions are invalid.
    [[nodiscard]] auto convert(const el::ByteBlock &data) -> el::StringMap<el::String>;
    /// Normalize response-file line endings and trailing horizontal whitespace.
    /// @param text Response text.
    /// @return Normalized response text, preserving blank lines.
    [[nodiscard]] static auto normalize(const el::String &text) -> el::String;
    /// Count response records and semantic groups for manifest generation.
    /// @param path Relative response-file path.
    /// @param text Response text with either LF or CRLF endings.
    /// @return Record and group counts.
    [[nodiscard]] static auto counts(const el::String &path, const el::String &text) -> std::pair<int64_t, int64_t>;

private:
    /// Extract selected response members and validate their expected dimensions.
    void convertArchive(const el::ByteBlock &data);
    /// Parse a size-bounded JSON source and dispatch by vector family.
    void convertJson(const el::ByteBlock &data);
    /// Convert the supported Wycheproof schemas, retaining negative payloads.
    void convertWycheproof(const el::json::JsonValue &root);
    /// Select the reviewed certificate validation cases by stable upstream identifiers.
    void convertLimbo(const el::json::JsonValue &root);
    /// Extract the complete unkeyed BLAKE2b-512 known-answer set.
    void convertBlake(const el::json::JsonValue &root);
    /// Select GCM groups with a 96-bit IV and a 128-bit tag.
    [[nodiscard]] auto filterGcm(const el::String &text) const -> el::String;
    /// Select HMAC groups for the requested output length in bytes.
    [[nodiscard]] auto filterHmac(const el::String &text, int length) const -> el::String;
    /// Convert NIST P-256/SHA-256 and P-384/SHA-384 signature verification records.
    [[nodiscard]] auto filterEcdsa(const el::String &text) const -> el::String;
    /// Assemble upstream attribution and a prominent conversion notice.
    [[nodiscard]] auto wycheproofHeader(const el::json::JsonValue &root, const el::String &title) const
        -> el::StringList;
    /// Render ordered groups with their header comments and exactly one final terminator.
    [[nodiscard]] auto render(const el::StringList &header, const el::render::ValueList &groups) const -> el::String;
    /// Require a named JSON text field, preserving source identity on failure.
    [[nodiscard]] auto text(const el::json::JsonValue &object, const el::String &key) const -> el::String;
    /// Require a named JSON integer field.
    [[nodiscard]] auto integer(const el::json::JsonValue &object, const el::String &key) const -> int64_t;
    /// Require an array value while sharing its immutable storage.
    [[nodiscard]] auto array(const el::json::JsonValue &value) const -> el::json::JsonArray;
    /// Join a test case's flag names in their upstream order.
    [[nodiscard]] auto flags(const el::json::JsonValue &test) const -> el::String;
    /// Prepare the common signature record fields without validating the signature.
    [[nodiscard]] auto signatureRecord(const el::json::JsonValue &test) const -> el::render::Value;
    /// Decode a PEM certificate envelope into hex without validating its DER payload.
    [[nodiscard]] auto certificate(const el::String &pem) const -> el::String;
    /// Create a structured name/value pair for the response template.
    [[nodiscard]] static auto field(const el::String &name, const el::String &value) -> el::render::Value;
    /// Create a structured response group with ordered headers and records.
    [[nodiscard]] static auto group(const el::render::ValueList &headers, const el::render::ValueList &records)
        -> el::render::Value;
    /// Format a signed scalar in decimal notation for a response field.
    [[nodiscard]] static auto decimal(int64_t value) -> el::String;
    /// Encode the supported RSA signature algorithm identifier and explicit PSS parameters.
    [[nodiscard]] auto algorithmDer(const el::String &padding, const el::String &sha, int64_t salt) const -> el::String;
    /// Canonically encode one nonnegative hexadecimal integer as a DER INTEGER.
    [[nodiscard]] auto derInteger(const el::String &value) const -> el::String;
    /// Report malformed or unsupported source data with its file identity.
    [[noreturn]] void fail(const el::String &description) const;

private:
    el::render::EnvironmentPtr _renderer; ///< Compiled text templates.
    VectorSource _source;                 ///< Source identity and family.
    el::String _revision;                 ///< Configured provenance revision.
    el::StringMap<el::String> _outputs;   ///< Complete proposed response files.
};

}
