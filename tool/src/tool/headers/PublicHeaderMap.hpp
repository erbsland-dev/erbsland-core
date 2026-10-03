// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/StringHashMap.hpp>
#include <erbsland/StringList.hpp>

namespace tool::headers {

/// Read the canonical-first publication map shared by maintenance commands.
class PublicHeaderMap final {
public: // parsing
    /// Replace the map after validating every record.
    /// @param text Complete mapping text.
    /// @param source Display path used in diagnostics.
    /// @throws el::ApplicationError If a path, ordering, or duplicate record is invalid.
    void read(const el::String &text, const el::String &source = {});

public: // accessors
    /// Get source-relative headers and their canonical public include paths.
    [[nodiscard]] auto canonical() const noexcept -> const el::StringHashMap<el::String> & { return _canonical; }
    /// Get all source headers published through each canonical or compatibility include.
    [[nodiscard]] auto sources() const noexcept -> const el::StringHashMap<el::StringList> & { return _sources; }

private:
    el::StringHashMap<el::String> _canonical;   ///< First include spelling for each source header.
    el::StringHashMap<el::StringList> _sources; ///< Sources of each public wrapper, including merged wrappers.
};

}
