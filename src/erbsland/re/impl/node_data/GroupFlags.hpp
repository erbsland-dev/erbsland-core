// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "GroupFlag.hpp"

#include "../../../util/EnumFlags.hpp"
#include "../../Flags.hpp"

#include <cstdint>

namespace erbsland::re::impl {

/// Flags for groups in a pattern
/// @tested{ParserFlagsAndVerboseTest}
class GroupFlags : public util::EnumFlags<GroupFlag, GroupFlags> {
    using Base = util::EnumFlags<GroupFlag, GroupFlags>;

public:
    using Base::Base;

public: // conversion
    /// Create a string with the usual flags representation
    /// This does not display the atomic flag.
    [[nodiscard]] auto toString() const -> text::String {
        text::StringEditor result;
        for (
            const auto &flag :
            {GroupFlag::IgnoreCase, GroupFlag::Multiline, GroupFlag::DotAll, GroupFlag::Ascii, GroupFlag::Verbose}) {
            if (isSet(flag)) {
                result.append(text::Char{static_cast<char32_t>(letterForFlag(flag))});
            }
        }
        return result;
    }

    /// Extract all relevant flags from the pattern flags.
    [[nodiscard]] static auto fromPatternFlags(const Flags flags) noexcept -> GroupFlags {
        auto result = GroupFlags{};
        if (flags.isSet(Flag::IgnoreCase)) {
            result.set(GroupFlag::IgnoreCase);
        }
        if (flags.isSet(Flag::Multiline)) {
            result.set(GroupFlag::Multiline);
        }
        if (flags.isSet(Flag::DotAll)) {
            result.set(GroupFlag::DotAll);
        }
        if (flags.isSet(Flag::Ascii)) {
            result.set(GroupFlag::Ascii);
        }
        if (flags.isSet(Flag::Verbose)) {
            result.set(GroupFlag::Verbose);
        }
        return result;
    }

private:
    /// Create a letter for a flag enum value.
    /// There is no letter for the atomic flag.
    [[nodiscard]] static auto letterForFlag(const GroupFlag flag) -> char {
        switch (flag) {
        case GroupFlag::IgnoreCase:
            return 'i';
        case GroupFlag::Multiline:
            return 'm';
        case GroupFlag::DotAll:
            return 's';
        case GroupFlag::Ascii:
            return 'a';
        case GroupFlag::Verbose:
            return 'x';
        default:
            return '?';
        }
    }
};

}
