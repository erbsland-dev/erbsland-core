// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../core/Definitions.hpp"

#include <bitset>
#include <cstdint>

namespace erbsland::conf {

/// Flags for rendering test output.
class TestFormat {
public:
    /// The enumeration with the individual flags.
    enum Flag : uint8_t {
        /// Show the size of a container in the type.
        /// If enabled, this will display the size of a container (e.g. <code>SectionWithNames(size=20)</code>)
        /// for all value types that can have children (sections, value lists).
        ShowContainerSize = 0,
        /// Show the position of a value.
        /// **Only when rendering value trees:**
        /// If enabled, the position of a value is added in square brackets after the value
        /// (e.g. <code>Integer(1)[1:2]</code>).
        ShowPosition,
        /// Show the source identifier of a value.
        /// **Only when rendering value trees:**
        /// If enabled, an identifier for the source is added after the value.
        /// The identifier is an upper case letter, like <code>Integer(1)</code>[A:1:2].
        /// The value-tree is followed by a legend, like <code>A: file:/path/example.elcl</code>
        ShowSourceIdentifier,

        _flagCount
    };

public:
    /// Create a test format with no flags set.
    TestFormat() = default;
    /// Create a test format with the given flag set.
    template <typename... Args>
        requires(std::is_same_v<Flag, Args> && ...)
    TestFormat(Args... flags) noexcept {
        static_assert(sizeof...(flags) <= _flagCount);
        (_flags.set(flags), ...);
    }
    // defaults
    ~TestFormat() = default;
    auto operator=(const TestFormat &) -> TestFormat & = default;
    auto operator=(TestFormat &&) -> TestFormat & = default;
    TestFormat(const TestFormat &) = default;
    TestFormat(TestFormat &&) = default;

public: // operators
    /// Compare for equality.
    auto operator==(const TestFormat &) const -> bool = default;
    /// Compare for unequality.
    auto operator!=(const TestFormat &) const -> bool = default;
    /// Combine two formats.
    [[nodiscard]] auto operator|(const TestFormat &other) const noexcept -> TestFormat {
        return TestFormat{_flags | other._flags};
    }
    /// Combine two formats.
    [[nodiscard]] auto operator|(const Flag flag) const noexcept -> TestFormat {
        auto flags = _flags;
        flags.set(flag);
        return TestFormat{flags};
    }
    /// Combine two formats.
    auto operator|=(const TestFormat &other) noexcept -> TestFormat & {
        _flags |= other._flags;
        return *this;
    }
    /// Combine two formats.
    auto operator|=(const Flag flag) noexcept -> TestFormat & {
        _flags.set(flag);
        return *this;
    }

public:
    /// Test if a flag is set.
    [[nodiscard]] auto isSet(const Flag flag) const noexcept -> bool { return _flags.test(flag); }

private:
    /// Create a format from an internal flag bit set.
    /// @param flags The flags to retain.
    explicit TestFormat(const std::bitset<_flagCount> &flags) : _flags(flags) {}

private:
    std::bitset<_flagCount> _flags;
};

}

/// Combine two test-format flags.
[[nodiscard]] inline auto operator|(
    const erbsland::conf::TestFormat::Flag left, const erbsland::conf::TestFormat::Flag right) noexcept
    -> erbsland::conf::TestFormat {
    auto value = erbsland::conf::TestFormat(left);
    value |= right;
    return value;
}

/// Add a test-format flag to a format value.
[[nodiscard]] inline auto operator|(
    const erbsland::conf::TestFormat::Flag left, const erbsland::conf::TestFormat right) noexcept
    -> erbsland::conf::TestFormat {
    auto value = right;
    value |= left;
    return value;
}
