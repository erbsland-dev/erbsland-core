// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>
#include <type_traits>

namespace erbsland::cterm {

/// A modifier pressed together with a key.
enum class KeyModifier : uint8_t {
    /// The Shift key.
    Shift = 1 << 0,
    /// The Control key.
    Control = 1 << 1,
    /// The Alt key.
    Alt = 1 << 2,
};

/// A set of key modifiers.
class KeyModifiers {
public:
    /// Unsigned storage type used for the combined modifier bits.
    using Mask = uint8_t;
    /// The enum type combined by this modifier set.
    using Enum = KeyModifier;

public:
    /// Create a combined set of modifiers.
    template <typename... Modifiers>
        requires(sizeof...(Modifiers) > 0 && (std::is_same_v<Enum, Modifiers> && ...))
    constexpr KeyModifiers(Modifiers... modifiers) : // NOLINT(*-explicit-constructor)
        _modifiers{static_cast<Mask>((static_cast<Mask>(modifiers) | ...))} {}

    // defaults
    KeyModifiers() = default;
    KeyModifiers(const KeyModifiers &) = default;
    auto operator=(const KeyModifiers &) -> KeyModifiers & = default;

public: // operators
    auto operator==(const KeyModifiers &other) const -> bool = default;
    auto operator!=(const KeyModifiers &other) const -> bool = default;
    /// Combine this modifier set with one additional modifier.
    /// @param modifier The modifier to add.
    /// @return The combined modifier set.
    auto operator|(const KeyModifier modifier) const -> KeyModifiers {
        return KeyModifiers{static_cast<Mask>(_modifiers | static_cast<Mask>(modifier))};
    }
    /// Combine one modifier with an existing modifier set.
    /// @param modifier The modifier to add.
    /// @param modifiers The existing modifier set.
    /// @return The combined modifier set.
    friend auto operator|(const KeyModifier modifier, const KeyModifiers modifiers) -> KeyModifiers {
        return KeyModifiers{static_cast<Mask>(modifiers._modifiers | static_cast<Mask>(modifier))};
    }
    /// Combine two modifier sets.
    /// @param modifiers1 The first modifier set.
    /// @param modifiers2 The second modifier set.
    /// @return The combined modifier set.
    friend auto operator|(const KeyModifiers modifiers1, const KeyModifiers modifiers2) -> KeyModifiers {
        return KeyModifiers{static_cast<Mask>(modifiers1._modifiers | modifiers2._modifiers)};
    }

public: // accessors
    /// Test if this modifier set is empty.
    [[nodiscard]] constexpr auto empty() const noexcept -> bool { return _modifiers == 0; }
    /// Test if a modifier is set.
    [[nodiscard]] constexpr auto has(const KeyModifier modifier) const noexcept -> bool {
        return (_modifiers & static_cast<Mask>(modifier)) != 0;
    }
    /// Access the raw modifier mask.
    [[nodiscard]] constexpr auto mask() const noexcept -> Mask { return _modifiers; }
    /// Set a modifier.
    void set(const KeyModifier modifier, const bool enabled = true) noexcept {
        if (enabled) {
            _modifiers |= static_cast<Mask>(modifier);
        } else {
            _modifiers &= static_cast<Mask>(~static_cast<Mask>(modifier));
        }
    }
    /// Clear a modifier.
    void clear(const KeyModifier modifier) noexcept { _modifiers &= static_cast<Mask>(~static_cast<Mask>(modifier)); }

private:
    explicit constexpr KeyModifiers(const Mask modifiers) : _modifiers{modifiers} {}

private:
    Mask _modifiers{0}; ///< The combined modifier bits.
};

/// Combine two key modifiers into one modifier set.
/// @param modifier1 The first modifier.
/// @param modifier2 The second modifier.
/// @return A modifier set containing both modifiers.
inline auto operator|(const KeyModifier modifier1, const KeyModifier modifier2) -> KeyModifiers {
    return KeyModifiers{modifier1, modifier2};
}

}
