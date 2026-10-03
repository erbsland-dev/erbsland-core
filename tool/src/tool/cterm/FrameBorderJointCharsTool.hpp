// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "FrameGeneratorTool.hpp"

namespace tool::cterm {

/// Generate the frame-border joint lookup table.
/// @tested{BoxFrameToolTest}
class FrameBorderJointCharsTool final : public FrameGeneratorTool {
public:
    /// Construct the command with its default Core output path.
    FrameBorderJointCharsTool();

public: // implement Tool
    [[nodiscard]] auto identifier() const -> el::String override;
    [[nodiscard]] auto title() const -> el::String override;
    [[nodiscard]] auto description() const -> el::String override;

protected: // implement FrameGeneratorTool
    [[nodiscard]] auto render(const BoxFrameData &data) const -> el::String override;
};

}
