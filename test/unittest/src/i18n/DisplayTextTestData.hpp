// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "DisplayTextTestEntry.hpp"

#include <erbsland/DisplayTextData.hpp>
#include <erbsland/StringMap.hpp>

#include <algorithm>
#include <initializer_list>
#include <memory>
#include <vector>

/// Owns sorted fixture arrays without exposing mutable registered data.
/// @notest{Immutable unit-test fixture.}
class DisplayTextTestData final : public el::i18n::DisplayTextData {
    /// Owns the entries for one immutable fixture domain.
    struct DomainStorage {
        el::text::StringLiteral name;                                ///< Permanent domain name.
        std::vector<el::i18n::impl::DisplayTextDomainEntry> entries; ///< Owned entry storage.
    };
    using DomainStoragePtr = std::shared_ptr<DomainStorage>;         ///< Stable ownership of each domain's entry array.

public:
    /// Copy fixture records and sort their domains and entries.
    /// @param entries Literal records to copy into immutable arrays.
    explicit DisplayTextTestData(std::initializer_list<DisplayTextTestEntry> entries) {
        for (const auto &entry : entries) {
            const auto name = el::text::String{entry.domain};
            auto domain = _storage.get(name, nullptr);
            if (!domain) {
                domain = std::make_shared<DomainStorage>(DomainStorage{entry.domain, {}});
                _storage.set(name, domain);
            }
            domain->entries.push_back({entry.key, entry.text});
        }
        for (const auto &[name, domain] : _storage) {
            std::ranges::sort(
                domain->entries, {}, [](const el::i18n::impl::DisplayTextDomainEntry &entry) -> el::text::String {
                    return el::text::String{entry.key};
                });
            _domains.push_back({domain->name, domain->entries});
        }
    }

public: // implement DisplayTextData
    [[nodiscard]] auto domains() const noexcept -> std::span<const el::i18n::impl::DisplayTextDomain> override {
        return _domains;
    }

private:
    el::StringMap<DomainStoragePtr> _storage;                ///< Domain storage ordered by name.
    std::vector<el::i18n::impl::DisplayTextDomain> _domains; ///< Immutable domain views.
};
