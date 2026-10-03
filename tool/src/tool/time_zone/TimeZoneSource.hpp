// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../frame/DataSource.hpp"

#include <erbsland/Literals.hpp>
#include <erbsland/Path.hpp>
#include <erbsland/StringMap.hpp>

namespace tool::time_zone {

using namespace el::text::literals;

/// Own the verified tzdb and CLDR source snapshots for time-zone generation.
/// @tested{TimeZoneDataToolTest}
class TimeZoneSource final : public frame::DataSource {
    /// Configuration key selecting the integrity-checked time-zone snapshot directory.
    constexpr static auto cInputDirectoryConfig = "input_directory"_el;
    /// Snapshot-relative path identifying the bundled IANA time-zone database version.
    constexpr static auto cVersionFile = "tzdb/version"_el;

public:
    // defaults
    TimeZoneSource() = default;
    ~TimeZoneSource() override = default;

public: // implement DataSource
    [[nodiscard]] auto configurationRules() const -> el::conf::vr::RulesPtr override;
    void configure(const el::conf::ValuePtr &configuration) override;
    void initialize() override;

public: // accessors
    /// Get one verified tzdb region or the CLDR XML source.
    [[nodiscard]] auto text(const el::String &name) const -> el::String;
    /// Get the version token read from tzdb/version.
    [[nodiscard]] auto version() const -> const el::String & { return _version; }
    /// Get the source registration identifier.
    [[nodiscard]] static auto sourceIdentifier() -> el::String;

private:
    el::Path _directory;              ///< Confined snapshot directory.
    el::String _version;              ///< Source-derived tzdb version.
    el::StringMap<el::String> _texts; ///< Hash-verified source texts.
};

}
