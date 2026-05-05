// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>
#include <type_traits>

namespace erbsland::cterm {

/// A terminal flag.
enum class TerminalFlag : uint8_t {
    /// Disables signal handling to restore the screen when the application is terminated.
    /// If this flag is set, you must ensure that the `restoreScreen()` method is called when the application
    /// is terminated by a signal. Otherwise, the terminal will not be restored properly.
    NoSignalHandling = 1 << 0,
};

/// A set of terminal flags.
/// Terminal flags control the behavior of the built-in terminal backend.
/// These flags can only be set at construction time and cannot be modified after that.
class TerminalFlags {
public:
    /// Unsigned storage type used for the combined flag bits.
    using Mask = uint8_t;
    /// The enum type combined by this flag set.
    using Enum = TerminalFlag;

public:
    /// Create a combined set of flags.
    template <typename... tFlags>
        requires(sizeof...(tFlags) > 0 && (std::is_same_v<Enum, tFlags> && ...))
    constexpr TerminalFlags(tFlags... flags) : // NOLINT(*-explicit-constructor)
        _flags{static_cast<Mask>((static_cast<Mask>(flags) | ...))} {}

    // defaults
    TerminalFlags() = default;
    TerminalFlags(const TerminalFlags &) = default;
    auto operator=(const TerminalFlags &) -> TerminalFlags & = default;

public:
    /// Combine this flag set with one additional flag.
    /// @param flag The flag to add.
    /// @return The combined flag set.
    auto operator|(const TerminalFlag flag) const -> TerminalFlags {
        return TerminalFlags{static_cast<Mask>(_flags | static_cast<Mask>(flag))};
    }
    /// Combine one flag with an existing flag set.
    /// @param flag The flag to add.
    /// @param flags The existing flag set.
    /// @return The combined flag set.
    friend auto operator|(const TerminalFlag flag, const TerminalFlags flags) -> TerminalFlags {
        return TerminalFlags{static_cast<Mask>(flags._flags | static_cast<Mask>(flag))};
    }
    /// Combine two flag sets.
    /// @param flags1 The first flag set.
    /// @param flags2 The second flag set.
    /// @return The combined flag set.
    friend auto operator|(const TerminalFlags flags1, const TerminalFlags flags2) -> TerminalFlags {
        return TerminalFlags{static_cast<Mask>(flags1._flags | flags2._flags)};
    }

public:
    /// Test if a flag is set.
    [[nodiscard]] auto has(const TerminalFlag flag) const noexcept -> bool {
        return (_flags & static_cast<Mask>(flag)) != 0;
    }
    /// Set a flag.
    void set(const TerminalFlag flag, const bool enabled = true) {
        if (enabled) {
            _flags |= static_cast<Mask>(flag);
        } else {
            _flags &= ~static_cast<Mask>(flag);
        }
    }
    /// Clear a flag.
    void clear(const TerminalFlag flag) { _flags &= ~static_cast<Mask>(flag); }

private:
    explicit TerminalFlags(const Mask flags) : _flags(flags) {}

private:
    Mask _flags{0};
};

/// Combine two terminal flags into one flag set.
/// @param flag1 The first terminal flag.
/// @param flag2 The second terminal flag.
/// @return A flag set containing both flags.
inline auto operator|(const TerminalFlag flag1, const TerminalFlag flag2) -> TerminalFlags {
    return TerminalFlags{flag1, flag2};
}

}
