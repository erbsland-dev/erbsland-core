// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "DisplayTextTestData.hpp"

#include <erbsland/Application.hpp>
#include <erbsland/DisplayText.hpp>

/// Registers immutable fixture data for this object's lifetime.
/// @notest{Scoped unit-test fixture.}
class DisplayTextTestLayer final {
public:
    /// Register fixture records in the current application.
    /// @param entries Immutable literal source records.
    explicit DisplayTextTestLayer(std::initializer_list<DisplayTextTestEntry> entries) :
        _registry{el::core::application().displayText()}, _data{std::make_shared<DisplayTextTestData>(entries)} {
        _registry->addData(_data);
    }
    /// Remove this layer while preserving the rest of the registry.
    ~DisplayTextTestLayer() { static_cast<void>(_registry->removeData(_data)); }

    // defaults/deletions
    DisplayTextTestLayer(const DisplayTextTestLayer &) = delete;
    auto operator=(const DisplayTextTestLayer &) -> DisplayTextTestLayer & = delete;

private:
    el::i18n::DisplayTextPtr _registry; ///< The registry used for registration.
    el::i18n::DisplayTextDataPtr _data; ///< Registered fixture data.
};
