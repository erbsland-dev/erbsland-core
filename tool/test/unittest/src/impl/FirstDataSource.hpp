// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/LogStream.hpp>
#include <tool/frame/DataSource.hpp>

#include <cstdint>

namespace tool::test::impl {

using namespace el::text::literals;

/// Configurable source fixture with observable lifecycle callbacks.
class FirstDataSource : public tool::frame::DataSource {
    static constexpr auto cConfiguredValue = "value"_el;

public: // implement DataSource
    [[nodiscard]] auto configurationRules() const -> el::conf::vr::RulesPtr override;
    void configure(const el::conf::ValuePtr &configuration) override;
    void initialize() override;

public: // fixture access
    /// Get the fixture's registration identifier.
    [[nodiscard]] static auto sourceIdentifier() -> el::String { return "first"_el; }
    /// Probe logging before framework initialization.
    [[nodiscard]] auto logging() const -> const el::LogStreamPtr & { return log(); }

public:
    std::int64_t value{};          ///< Value applied by source validation.
    bool hasConfiguration = false; ///< Whether configure received a section.

protected:                         // fixture identity
    /// Get the label used to distinguish lifecycle events.
    [[nodiscard]] virtual auto label() const -> el::String { return "first"_el; }
};

}
