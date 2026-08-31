// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::system {

/// An opaque identity for one filesystem object.
/// @tested{PathInfoTest PathContentTest}
class FileIdentity final {
public:
    /// Create an invalid file identity.
    FileIdentity() = default;
    /// Create a valid identity from two platform-specific values.
    /// The values remain opaque and are meaningful only for equality comparison.
    /// @param first The first native identity component.
    /// @param second The second native identity component.
    /// @return A valid identity containing both native components.
    [[nodiscard]] static constexpr auto fromNativeValues(const uint64_t first, const uint64_t second) noexcept
        -> FileIdentity {
        return FileIdentity{first, second};
    }

    // defaults
    ~FileIdentity() = default;
    FileIdentity(const FileIdentity &) = default;
    FileIdentity(FileIdentity &&) noexcept = default;
    auto operator=(const FileIdentity &) -> FileIdentity & = default;
    auto operator=(FileIdentity &&) noexcept -> FileIdentity & = default;

public: // operators
    auto operator==(const FileIdentity &other) const noexcept -> bool = default;

public: // accessors
    /// Test if this identity represents a filesystem object.
    [[nodiscard]] constexpr auto isValid() const noexcept -> bool { return _valid; }

private:
    /// Create a valid identity from native values.
    /// @param first The first opaque native component.
    /// @param second The second opaque native component.
    constexpr FileIdentity(const uint64_t first, const uint64_t second) noexcept :
        _first{first}, _second{second}, _valid{true} {}

private:
    uint64_t _first{};  ///< First opaque platform value.
    uint64_t _second{}; ///< Second opaque platform value.
    bool _valid{false}; ///< Whether this identity is valid.
};

}
