// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "IdnaMapping.hpp"
#include "NfcNormalizer.hpp"
#include "UnicodeData.hpp"

#include "../idna/IdnaData.hpp"

#include <erbsland/Literals.hpp>
#include <tool/frame/DataSource.hpp>

#include <memory>

namespace tool::unicode {

using namespace el::text::literals;

/// Shared Unicode snapshot and source-derived processing data for maintenance commands.
/// @tested{UnicodeSourceFullTest UnicodeSourceTest DataSourceTest}
class UnicodeSource final : public frame::DataSource {
    /// Configuration key selecting the confined Unicode Character Database input directory.
    constexpr static auto cUcdDirectoryConfig = "ucd_directory"_el;
    /// Configuration key pinning the expected Unicode snapshot version.
    constexpr static auto cUnicodeVersionConfig = "unicode_version"_el;

public:
    // defaults
    UnicodeSource() = default;
    ~UnicodeSource() override = default;

public: // implement DataSource
    [[nodiscard]] auto configurationRules() const -> el::conf::vr::RulesPtr override;
    void configure(const el::conf::ValuePtr &configuration) override;
    void initialize() override;

public: // accessors
    /// Get the confined directory selected during configuration.
    [[nodiscard]] auto directory() const -> const el::Path & { return _directory; }
    /// Get the explicit expected Unicode version.
    [[nodiscard]] auto version() const -> const el::String & { return _version; }
    /// Get the verified production input texts.
    /// Initialization must have completed successfully.
    [[nodiscard]] auto dataset() const -> const UcdDataset &;
    /// Get the shared validated UnicodeData records.
    /// Initialization must have completed successfully.
    [[nodiscard]] auto unicodeData() const -> const UnicodeData &;
    /// Get source-derived RFC 5892 status and contextual attributes.
    /// Initialization must have completed successfully.
    [[nodiscard]] auto idnaData() const -> const idna::IdnaData &;
    /// Get source-backed NFC processing.
    /// Initialization must have completed successfully.
    [[nodiscard]] auto normalizer() const -> const NfcNormalizer &;
    /// Get source-backed UTS #46 mapping.
    /// Initialization must have completed successfully.
    [[nodiscard]] auto mapping() const -> const IdnaMapping &;

public: // identity
    /// Get the shared configuration and registration identifier.
    [[nodiscard]] static auto sourceIdentifier() -> el::String;

private:
    /// Guard prepared-data access until every production component is available.
    /// Initialization must have completed successfully.
    void requireInitialized() const;

private:
    el::Path _directory;                        ///< Confined UCD input directory.
    el::String _version;                        ///< Configured version pin.
    std::unique_ptr<UcdDataset> _dataset;       ///< Verified production input texts.
    std::unique_ptr<UnicodeData> _unicodeData;  ///< Shared validated records.
    std::unique_ptr<idna::IdnaData> _idnaData;  ///< Source-derived IDNA attributes.
    std::unique_ptr<NfcNormalizer> _normalizer; ///< Source-derived normalization tables.
    std::unique_ptr<IdnaMapping> _mapping;      ///< Mapping tables; present only when all preparation succeeded.
};

}
