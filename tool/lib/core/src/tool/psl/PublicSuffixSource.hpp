// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/LineIndex.hpp>
#include <erbsland/Literals.hpp>
#include <erbsland/Path.hpp>
#include <erbsland/String.hpp>
#include <tool/frame/DataSource.hpp>

namespace tool::psl {

using namespace el::text::literals;

/// Shared, integrity-checked Public Suffix List snapshot.
/// @tested{PublicSuffixFullTest PublicSuffixSourceTest DataSourceTest}
class PublicSuffixSource final : public frame::DataSource {
    /// Configuration key selecting the integrity-checked Public Suffix List snapshot.
    constexpr static auto cInputFileConfig = "input_file"_el;

public:
    // defaults
    PublicSuffixSource() = default;
    ~PublicSuffixSource() override = default;

public: // implement DataSource
    [[nodiscard]] auto configurationRules() const -> el::conf::vr::RulesPtr override;
    void configure(const el::conf::ValuePtr &configuration) override;
    void initialize() override;

public: // accessors
    /// Get the confined input path selected during configuration.
    [[nodiscard]] auto inputFile() const -> const el::Path & { return _inputFile; }
    /// Get the verified snapshot as tolerant UTF-8 with automatic BOM handling.
    /// Initialization must have completed successfully.
    [[nodiscard]] auto text() const -> const el::String &;
    /// Get the version verified against both the manifest and source header.
    /// Initialization must have completed successfully.
    [[nodiscard]] auto version() const -> const el::String &;

public: // identity
    /// Get the shared configuration and registration identifier.
    [[nodiscard]] static auto sourceIdentifier() -> el::String;

private:
    /// Guard access to verified text and version metadata.
    /// Initialization must have completed successfully.
    void requireInitialized() const;
    /// Require a valid manifest version and exactly one matching snapshot header.
    /// @param expected The version recorded in the sibling manifest.
    /// @throws el::ApplicationError If syntax or snapshot headers do not match.
    void validateVersion(const el::String &expected) const;
    /// Report a snapshot validation failure with source context.
    /// @param description The reason the snapshot is invalid.
    /// @param line The source line when known.
    [[noreturn]] void throwSnapshotError(const el::String &description, el::LineIndex line = {}) const;

private:
    el::Path _inputFile;       ///< Confined pinned snapshot path.
    el::String _text;          ///< Source payload after BOM handling.
    el::String _version;       ///< Version declared by the validated manifest.
    bool _initialized = false; ///< True after all source validation succeeded.
};

}
