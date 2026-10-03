// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Settings.hpp"

#include "../frame/PreCommitTask.hpp"
#include "../frame/Tool.hpp"

#include <erbsland/Literals.hpp>

namespace tool::security_hashes {

using namespace el::text::literals;

/// Generate or verify portable hashes of the selected infrastructure sources.
class SecurityHashesTool final : public frame::Tool, public frame::PreCommitTask {
    /// Option identifying read-only manifest verification.
    constexpr static auto cVerifyOption = "verify"_el;
    /// Option selecting the maximum number of file workers.
    constexpr static auto cJobsOption = "jobs"_el;

public: // implement Tool
    [[nodiscard]] auto identifier() const -> el::String override;
    [[nodiscard]] auto title() const -> el::String override;
    [[nodiscard]] auto description() const -> el::String override;
    [[nodiscard]] auto optionModule() const -> el::OptionModulePtr override;
    [[nodiscard]] auto configurationRules() const -> el::conf::vr::RulesPtr override;
    void configure(const el::conf::ValuePtr &configuration, const el::OptionValuesPtr &values) override;
    void execute() override;

private:
    Settings _settings;       ///< Validated source selection and destination.
    unsigned int _workers{1}; ///< Maximum concurrent file jobs.
    bool _verify{false};      ///< Compare without modifying the manifest.
};

}
