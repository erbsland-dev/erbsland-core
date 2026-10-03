// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "BoxFrameData.hpp"

#include "../frame/DataSource.hpp"

#include <erbsland/Literals.hpp>
#include <erbsland/Path.hpp>

#include <memory>

namespace tool::cterm {

using namespace el::text::literals;

/// Load the selected terminal frame repertoire and interpret its verified UCD names.
/// @tested{BoxFrameToolTest}
class BoxFrameSource final : public frame::DataSource {
    /// Configuration key selecting the source-backed terminal frame repertoire.
    constexpr static auto cInputFileConfig = "input_file"_el;

public:
    // defaults
    BoxFrameSource() = default;
    ~BoxFrameSource() override = default;

public: // implement DataSource
    [[nodiscard]] auto configurationRules() const -> el::conf::vr::RulesPtr override;
    void configure(const el::conf::ValuePtr &configuration) override;
    void initialize() override;

public: // accessors
    /// Get source-backed frame character data after initialization.
    [[nodiscard]] auto data() const -> const BoxFrameData &;
    /// Get this source's registration identifier.
    [[nodiscard]] static auto sourceIdentifier() -> el::String;

private:
    el::Path _inputFile;                 ///< Selected repertoire file.
    std::unique_ptr<BoxFrameData> _data; ///< Validated and interpreted characters.
};

}
