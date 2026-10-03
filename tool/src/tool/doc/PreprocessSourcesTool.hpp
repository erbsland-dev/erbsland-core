// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../frame/Tool.hpp"

namespace tool::doc {

/// Prepare a cached Doxygen-compatible source tree.
class PreprocessSourcesTool final : public frame::Tool {
public:
    // defaults
    PreprocessSourcesTool() = default;

public: // implement Tool
    auto identifier() const -> el::String override;
    auto title() const -> el::String override;
    auto description() const -> el::String override;
    auto optionModule() const -> el::OptionModulePtr override;
    void configure(const el::conf::ValuePtr &configuration, const el::OptionValuesPtr &values) override;
    void execute() override;

private:
    bool _force{false}; ///< Regenerate cached input even if unchanged.
};

}
