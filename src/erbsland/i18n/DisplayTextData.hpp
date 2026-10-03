// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "DisplayTextData_fwd.hpp"

#include "impl/DisplayTextDomain.hpp"

#include "../text/String.hpp"

#include <optional>
#include <span>

namespace erbsland::i18n {

/// Immutable display-text source data with binary lookup over sorted literal arrays.
/// @seedoc{/reference/i18n/internationalization}
/// @tested{DisplayTextDataTest}
class DisplayTextData {
protected:
    // defaults
    DisplayTextData() = default;

public:
    // defaults/deletions
    virtual ~DisplayTextData() = default;
    DisplayTextData(const DisplayTextData &) = delete;
    DisplayTextData(DisplayTextData &&) = delete;
    auto operator=(const DisplayTextData &) -> DisplayTextData & = delete;
    auto operator=(DisplayTextData &&) -> DisplayTextData & = delete;

public: // access
    /// Access sorted domains and their sorted entries, valid for this instance's lifetime.
    /// All names and entries must be immutable and use normalized ELCL names.
    [[nodiscard]] virtual auto domains() const noexcept -> std::span<const impl::DisplayTextDomain> = 0;
    /// Look up an exact normalized key without registry fallback or translation.
    /// @param key A normalized dot-separated key.
    /// @return The source text, including an empty text, or no value if absent.
    [[nodiscard]] auto findText(const text::String &key) const -> std::optional<text::String>;
};

}
