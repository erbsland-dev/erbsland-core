// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "impl/HashHelper.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>

namespace erbsland::cterm {

/// A set of optional ANSI character attributes.
/// Each attribute stores two states: whether it is explicitly specified and whether it is enabled.
/// Unspecified attributes inherit the state from the surrounding writer or character below.
class BlockAttributes final {
public:
    /// A single character attribute flag.
    struct Flag {
        uint8_t value; ///< The bit mask value for the flag.
    };

    /// Bold text.
    constexpr static auto Bold = Flag{0x01U};
    /// Dim text.
    constexpr static auto Dim = Flag{0x02U};
    /// Italic text.
    constexpr static auto Italic = Flag{0x04U};
    /// Underlined text.
    constexpr static auto Underline = Flag{0x08U};
    /// Blinking text.
    constexpr static auto Blink = Flag{0x10U};
    /// Reverse foreground/background display.
    constexpr static auto Reverse = Flag{0x20U};
    /// Hidden text.
    constexpr static auto Hidden = Flag{0x40U};
    /// Strikethrough text.
    constexpr static auto Strikethrough = Flag{0x80U};

private:
    constexpr static auto cAllMask = uint8_t{0xFFU};

public:
    /// Create attributes with no explicitly specified flags.
    constexpr BlockAttributes() noexcept = default;
    /// Create attributes with one enabled flag.
    /// @param flag The flag to enable and specify.
    constexpr BlockAttributes(const Flag flag) noexcept : _enabledMask{flag.value}, _specifiedMask{flag.value} {}

    // defaults
    ~BlockAttributes() = default;
    BlockAttributes(const BlockAttributes &) = default;
    BlockAttributes(BlockAttributes &&) = default;
    auto operator=(const BlockAttributes &) -> BlockAttributes & = default;
    auto operator=(BlockAttributes &&) -> BlockAttributes & = default;

public: // operators
    /// Compare two attribute sets for equality.
    auto operator==(const BlockAttributes &) const noexcept -> bool = default;
    /// Compare two attribute sets for inequality.
    auto operator!=(const BlockAttributes &) const noexcept -> bool = default;

public: // accessors
    /// Get the enabled bit mask.
    /// Only specified bits are relevant.
    [[nodiscard]] constexpr auto enabledMask() const noexcept -> uint8_t { return _enabledMask; }
    /// Get the specified bit mask.
    [[nodiscard]] constexpr auto specifiedMask() const noexcept -> uint8_t { return _specifiedMask; }
    /// Get the mask of attributes that are both specified and enabled.
    [[nodiscard]] constexpr auto mask() const noexcept -> uint8_t { return _enabledMask & _specifiedMask; }

public: // tools
    /// Create attributes from an enabled/specified mask pair.
    /// @param enabledMask The enabled bits.
    /// @param specifiedMask The specified bits.
    /// @return The new attribute set.
    [[nodiscard]] constexpr static auto fromMasks(uint8_t enabledMask, uint8_t specifiedMask) noexcept
        -> BlockAttributes {
        return BlockAttributes{static_cast<uint8_t>(enabledMask & specifiedMask), specifiedMask};
    }
    /// Create a fully specified attribute mask from enabled bits.
    /// @param mask The enabled bits.
    /// @return The fully specified attribute set.
    [[nodiscard]] constexpr static auto fromMask(const uint8_t mask) noexcept -> BlockAttributes {
        return fromMasks(mask, mask);
    }
    /// Create a fully specified attribute set with all flags disabled.
    /// @return The reset attribute set.
    [[nodiscard]] constexpr static auto reset() noexcept -> BlockAttributes { return fromMasks(0, cAllMask); }
    /// Create a fully specified attribute set with all flags enabled.
    /// @return The fully enabled attribute set.
    [[nodiscard]] constexpr static auto all() noexcept -> BlockAttributes { return fromMask(cAllMask); }
    /// Test whether a flag is explicitly specified.
    /// @param flag The flag to test.
    /// @return `true` if the flag is explicitly specified.
    [[nodiscard]] constexpr auto isSpecified(const Flag flag) const noexcept -> bool {
        return (_specifiedMask & flag.value) != 0;
    }
    /// Test whether a flag is specified and enabled.
    /// @param flag The flag to test.
    /// @return `true` if the flag is specified and enabled.
    [[nodiscard]] constexpr auto isEnabled(const Flag flag) const noexcept -> bool {
        return (_specifiedMask & flag.value) != 0 && (_enabledMask & flag.value) != 0;
    }
    /// Apply these attributes on top of a base attribute set.
    /// Only explicitly specified attributes overwrite the base state.
    /// @param base The base attributes.
    /// @return The resolved attribute set.
    [[nodiscard]] constexpr auto withBase(const BlockAttributes base) const noexcept -> BlockAttributes {
        return fromMasks(
            static_cast<uint8_t>((base._enabledMask & ~_specifiedMask) | (_enabledMask & _specifiedMask)),
            static_cast<uint8_t>(base._specifiedMask | _specifiedMask));
    }
    /// Create a copy with one flag explicitly enabled or disabled.
    /// @param flag The flag to change.
    /// @param enabled `true` to enable the flag, `false` to disable it.
    /// @return The updated attribute set.
    [[nodiscard]] constexpr auto withFlag(const Flag flag, const bool enabled) const noexcept -> BlockAttributes {
        auto result = *this;
        result.setFlag(flag, enabled);
        return result;
    }
    /// Get a stable hash for the attribute set.
    [[nodiscard]] constexpr auto hash() const noexcept -> std::size_t {
        return impl::hashCreate(_enabledMask, _specifiedMask);
    }

public: // per-attribute tests
    /// Test if the bold attribute is specified.
    [[nodiscard]] constexpr auto isBoldSpecified() const noexcept -> bool { return isSpecified(Bold); }
    /// Test if the bold attribute is enabled.
    [[nodiscard]] constexpr auto isBold() const noexcept -> bool { return isEnabled(Bold); }
    /// Test if the dim attribute is specified.
    [[nodiscard]] constexpr auto isDimSpecified() const noexcept -> bool { return isSpecified(Dim); }
    /// Test if the dim attribute is enabled.
    [[nodiscard]] constexpr auto isDim() const noexcept -> bool { return isEnabled(Dim); }
    /// Test if the italic attribute is specified.
    [[nodiscard]] constexpr auto isItalicSpecified() const noexcept -> bool { return isSpecified(Italic); }
    /// Test if the italic attribute is enabled.
    [[nodiscard]] constexpr auto isItalic() const noexcept -> bool { return isEnabled(Italic); }
    /// Test if the underline attribute is specified.
    [[nodiscard]] constexpr auto isUnderlineSpecified() const noexcept -> bool { return isSpecified(Underline); }
    /// Test if the underline attribute is enabled.
    [[nodiscard]] constexpr auto isUnderline() const noexcept -> bool { return isEnabled(Underline); }
    /// Test if the blink attribute is specified.
    [[nodiscard]] constexpr auto isBlinkSpecified() const noexcept -> bool { return isSpecified(Blink); }
    /// Test if the blink attribute is enabled.
    [[nodiscard]] constexpr auto isBlink() const noexcept -> bool { return isEnabled(Blink); }
    /// Test if the reverse attribute is specified.
    [[nodiscard]] constexpr auto isReverseSpecified() const noexcept -> bool { return isSpecified(Reverse); }
    /// Test if the reverse attribute is enabled.
    [[nodiscard]] constexpr auto isReverse() const noexcept -> bool { return isEnabled(Reverse); }
    /// Test if the hidden attribute is specified.
    [[nodiscard]] constexpr auto isHiddenSpecified() const noexcept -> bool { return isSpecified(Hidden); }
    /// Test if the hidden attribute is enabled.
    [[nodiscard]] constexpr auto isHidden() const noexcept -> bool { return isEnabled(Hidden); }
    /// Test if the strikethrough attribute is specified.
    [[nodiscard]] constexpr auto isStrikethroughSpecified() const noexcept -> bool {
        return isSpecified(Strikethrough);
    }
    /// Test if the strikethrough attribute is enabled.
    [[nodiscard]] constexpr auto isStrikethrough() const noexcept -> bool { return isEnabled(Strikethrough); }

public: // per-attribute modifiers
    /// Set or clear the bold attribute explicitly.
    /// @param enabled `true` to enable the attribute, `false` to disable it.
    constexpr void setBold(const bool enabled) noexcept { setFlag(Bold, enabled); }
    /// Make the bold attribute inherit from the base state.
    constexpr void setBoldInherited() noexcept { setInheritedFlag(Bold); }
    /// Set or clear the dim attribute explicitly.
    /// @param enabled `true` to enable the attribute, `false` to disable it.
    constexpr void setDim(const bool enabled) noexcept { setFlag(Dim, enabled); }
    /// Make the dim attribute inherit from the base state.
    constexpr void setDimInherited() noexcept { setInheritedFlag(Dim); }
    /// Set or clear the italic attribute explicitly.
    /// @param enabled `true` to enable the attribute, `false` to disable it.
    constexpr void setItalic(const bool enabled) noexcept { setFlag(Italic, enabled); }
    /// Make the italic attribute inherit from the base state.
    constexpr void setItalicInherited() noexcept { setInheritedFlag(Italic); }
    /// Set or clear the underline attribute explicitly.
    /// @param enabled `true` to enable the attribute, `false` to disable it.
    constexpr void setUnderline(const bool enabled) noexcept { setFlag(Underline, enabled); }
    /// Make the underline attribute inherit from the base state.
    constexpr void setUnderlineInherited() noexcept { setInheritedFlag(Underline); }
    /// Set or clear the blink attribute explicitly.
    /// @param enabled `true` to enable the attribute, `false` to disable it.
    constexpr void setBlink(const bool enabled) noexcept { setFlag(Blink, enabled); }
    /// Make the blink attribute inherit from the base state.
    constexpr void setBlinkInherited() noexcept { setInheritedFlag(Blink); }
    /// Set or clear the reverse attribute explicitly.
    /// @param enabled `true` to enable the attribute, `false` to disable it.
    constexpr void setReverse(const bool enabled) noexcept { setFlag(Reverse, enabled); }
    /// Make the reverse attribute inherit from the base state.
    constexpr void setReverseInherited() noexcept { setInheritedFlag(Reverse); }
    /// Set or clear the hidden attribute explicitly.
    /// @param enabled `true` to enable the attribute, `false` to disable it.
    constexpr void setHidden(const bool enabled) noexcept { setFlag(Hidden, enabled); }
    /// Make the hidden attribute inherit from the base state.
    constexpr void setHiddenInherited() noexcept { setInheritedFlag(Hidden); }
    /// Set or clear the strikethrough attribute explicitly.
    /// @param enabled `true` to enable the attribute, `false` to disable it.
    constexpr void setStrikethrough(const bool enabled) noexcept { setFlag(Strikethrough, enabled); }
    /// Make the strikethrough attribute inherit from the base state.
    constexpr void setStrikethroughInherited() noexcept { setInheritedFlag(Strikethrough); }

public: // deprecated methods
    /// Return these attributes resolved against a base state.
    /// @param base The base attributes.
    /// @return The resolved attributes.
    /// @deprecated Use ``withBase()`` instead.
    [[deprecated("Please use withBase(base)"), nodiscard]]
    constexpr auto resolvedWith(const BlockAttributes base) const noexcept -> BlockAttributes {
        return withBase(base);
    }

private:
    constexpr BlockAttributes(const uint8_t enabledMask, const uint8_t specifiedMask) noexcept :
        _enabledMask{enabledMask}, _specifiedMask{specifiedMask} {}
    constexpr void setFlag(const Flag flag, const bool enabled) noexcept {
        _specifiedMask |= flag.value;
        if (enabled) {
            _enabledMask |= flag.value;
        } else {
            _enabledMask &= static_cast<uint8_t>(~flag.value);
        }
    }
    constexpr void setInheritedFlag(const Flag flag) noexcept {
        _specifiedMask &= static_cast<uint8_t>(~flag.value);
        _enabledMask &= static_cast<uint8_t>(~flag.value);
    }

private:
    uint8_t _enabledMask{0};   ///< The explicitly enabled attributes.
    uint8_t _specifiedMask{0}; ///< The explicitly specified attributes.
};

}

template <>
struct std::hash<erbsland::cterm::BlockAttributes> {
    auto operator()(const erbsland::cterm::BlockAttributes &attributes) const noexcept -> std::size_t {
        return attributes.hash();
    }
};
