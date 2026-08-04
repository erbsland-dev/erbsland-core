// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "AbbreviationOffset.hpp"
#include "Database_fwd.hpp"
#include "Info.hpp"
#include "TextId.hpp"
#include "ZoneId.hpp"
#include "ZoneName.hpp"

#include "../../../text/String.hpp"
#include "../../../text/StringEditor.hpp"
#include "../../../text/StringList.hpp"
#include "../../../unit/Version.hpp"

#include <map>
#include <memory>
#include <shared_mutex>

namespace erbsland::time::tz::impl {

/// Access to the generated IANA time-zone database.
///
/// Provides name lookups, abbreviation resolution, and zone info access.
/// @notest{Internal database adapter tested through public time-zone tests.}
class Database final {
public:
    /// Return all zone names.
    /// @return A list of all IANA zone names.
    [[nodiscard]] auto names() const -> text::StringList;
    /// Test if a zone name exists.
    /// @param zoneName The zone name to check.
    /// @return `true` if the zone is known.
    [[nodiscard]] auto hasName(const text::String &zoneName) const noexcept -> bool;
    /// Look up a zone identifier by name.
    /// @param zoneName The zone name.
    /// @return The zone identifier, or `cZoneIdNotFound` if unknown.
    [[nodiscard]] auto zoneIdFromName(const text::String &zoneName) const noexcept -> ZoneId;
    /// Look up a zone name by identifier.
    /// @param zoneId The zone identifier.
    /// @return The zone name.
    [[nodiscard]] auto nameFromZoneId(ZoneId zoneId) const -> text::String;
    /// Look up an abbreviation by offset.
    /// @param zoneId The zone identifier.
    /// @param abbreviationOffset The abbreviation offset.
    /// @return The abbreviation text.
    [[nodiscard]] auto abbreviation(ZoneId zoneId, AbbreviationOffset abbreviationOffset) const -> text::String;
    /// Get zone info.
    /// @param zoneId The zone identifier.
    /// @return The zone info, or `nullptr` if not found.
    [[nodiscard]] auto info(ZoneId zoneId) const noexcept -> std::shared_ptr<const Info>;

public:
    /// Return the database version.
    /// @return The version of the bundled database.
    [[nodiscard]] static auto version() noexcept -> unit::Version;
    /// Look up a text entry by index.
    /// @param index The text index.
    /// @return The text string.
    [[nodiscard]] static auto textFromIndex(TextId index) noexcept -> text::String;
    /// Look up a text index by string.
    /// @param text The text string.
    /// @return The text index, or a sentinel value if not found.
    [[nodiscard]] static auto indexFromText(const text::String &text) noexcept -> TextId;

private:
    /// Convert a generated zone name to a string.
    /// @param zoneName The generated zone name.
    /// @return The corresponding IANA zone name.
    [[nodiscard]] static auto zoneNameToString(const ZoneName &zoneName) -> text::String;

private:
    mutable std::shared_mutex _infoMutex;
    mutable std::map<ZoneId, std::shared_ptr<const Info>> _infoCache;
};

}
