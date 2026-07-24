// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <TerminalApplication.hpp>

#include <array>

namespace demo {

/// Display the ANSI character attributes supported by the current terminal setup.
class DisplayAllAttributesApp final : public TerminalApplication {
public:
    using TerminalApplication::TerminalApplication;

public:
    /// Render the attribute overview once and exit the demo.
    auto beforeMain() -> int override;

private:
    /// One attribute row in the demo output.
    struct AttributeSpec {
        el::String name;            ///< The display name for the row.
        BlockAttributes::Flag flag; ///< The represented attribute.
        el::String sgrCodes;        ///< The related ANSI SGR codes.
        el::String note;            ///< A short note for the row.
        el::String sampleText;      ///< The sample text rendered with the attribute.
    };

private:
    [[nodiscard]] static auto attributeSpecs() -> const std::array<AttributeSpec, 8> &;
    void printHeader() noexcept;
    void printAttributeTable() noexcept;
    void printAttributeRow(const AttributeSpec &spec, bool supported) noexcept;
    void printCombinations() noexcept;
    void printPausePrompt() noexcept;
    [[nodiscard]] static auto padded(el::String text, std::size_t width) -> el::String;
    [[nodiscard]] static auto supportLabel(bool supported) -> el::String;
    [[nodiscard]] static auto sampleAttributes(BlockAttributes::Flag flag) -> BlockAttributes;
};

}
