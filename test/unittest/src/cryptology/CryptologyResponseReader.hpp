// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/MakeOneNamespace.hpp>
#include <erbsland/path/Path.hpp>
#include <erbsland/text/String.hpp>
#include <erbsland/text/StringMap.hpp>

#include <cstddef>
#include <initializer_list>
#include <utility>
#include <vector>

/// A strict reader for NIST-style cryptographic response files.
/// @tested{AesCbcFullTest AesGcmFullTest HashFullValidationTest}
class CryptologyResponseReader final {
public:
    /// The operation direction associated with a response record.
    enum class Direction {
        None,    ///< The response file does not specify a direction.
        Encrypt, ///< The record validates encryption.
        Decrypt, ///< The record validates decryption.
    };

    /// One parsed response-file record with inherited group settings.
    struct Record final {
        using Values = el::StringMap<el::String>; ///< Response names and their values.

        el::Path path;                            ///< The source fixture path.
        std::size_t line{};                       ///< The first source line of this record.
        std::size_t index{};                      ///< The zero-based record index.
        Direction direction{Direction::None};     ///< The active operation direction.
        Values group;                             ///< Group settings active for this record.
        Values values;                            ///< Values stored in this record.
        bool failed{};                            ///< Whether the record contains the standalone `FAIL` marker.

        /// Test whether the record contains a value.
        [[nodiscard]] auto hasValue(const el::String &name) const -> bool;
        /// Return a required response value.
        /// @throws el::RuntimeError If the value is missing.
        [[nodiscard]] auto value(const el::String &name) const -> el::String;
        /// Return a required group setting.
        /// @throws el::RuntimeError If the setting is missing.
        [[nodiscard]] auto setting(const el::String &name) const -> el::String;
        /// Parse a required response value as an unsigned integer.
        /// @throws el::RuntimeError If the value is missing or malformed.
        [[nodiscard]] auto unsignedValue(const el::String &name) const -> std::size_t;
        /// Parse a required group setting as an unsigned integer.
        /// @throws el::RuntimeError If the setting is missing or malformed.
        [[nodiscard]] auto unsignedSetting(const el::String &name) const -> std::size_t;
        /// Require all listed response values.
        /// @throws el::RuntimeError If any value is missing.
        void requireValues(std::initializer_list<el::String> names) const;
        /// Reject response values outside the supplied set.
        /// @throws el::RuntimeError If an unknown value is present.
        void requireAllowedValues(std::initializer_list<el::String> names) const;
        /// Reject inherited group settings outside the supplied set.
        /// @throws el::RuntimeError If an unknown setting is present.
        void requireAllowedSettings(std::initializer_list<el::String> names) const;
        /// Build a concise diagnostic label for this record.
        [[nodiscard]] auto diagnostic() const -> el::String;

    private:
        /// Return a required entry from a value map.
        [[nodiscard]] auto required(const Values &source, const el::String &name, const el::String &kind) const
            -> el::String;
        /// Parse a decimal unsigned integer without accepting trailing characters.
        [[nodiscard]] auto unsignedInteger(const el::String &text) const -> std::size_t;
        /// Reject map keys outside the supplied set.
        void requireAllowed(
            const Values &source, std::initializer_list<el::String> names, const el::String &kind) const;
    };

public:
    /// Create a reader for a copied unit-test data file.
    explicit CryptologyResponseReader(const el::Path &relativePath);
    /// @overload
    explicit CryptologyResponseReader(const el::String &relativePath) :
        CryptologyResponseReader{el::Path{relativePath}} {}

    /// Read all records from the fixture.
    /// @return All strictly parsed records.
    /// @throws el::RuntimeError If any non-comment input line is malformed.
    [[nodiscard]] auto read() const -> std::vector<Record>;

private:
    /// Resolve a copied unit-test data path relative to the test executable.
    [[nodiscard]] static auto resolveDataPath(const el::Path &relativePath) -> el::Path;
    /// Split a response-file assignment into trimmed name and value parts.
    [[nodiscard]] auto assignment(const el::String &line, std::size_t lineNumber) const
        -> std::pair<el::String, el::String>;

private:
    el::Path _path;
};
