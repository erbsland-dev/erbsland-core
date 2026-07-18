// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "impl/StringReaderBackendKind.hpp"
#include "impl/StringReaderBase_fwd.hpp"

#include "../mem/StorageIdentifier.hpp"
#include "../unit/CpIndex.hpp"

#include <cstddef>

namespace erbsland::text {

/// A saved state for `StringReader`.
///
/// This value is intentionally opaque. It stores the reader backend kind, the visible storage identity, the backend
/// cursor, and the decoded code-point position. Create it via `StringCharReader::save()` and pass it back to
/// `StringCharReader::restore()`.
/// @seedoc{/reference/text/string_reader}
/// @tested{StringCharReaderTest}
class StringCharReaderState final {
    friend class StringCharReader;
    friend class impl::StringReaderBase;

public:
    StringCharReaderState() = default;
    ~StringCharReaderState() = default;
    StringCharReaderState(const StringCharReaderState &) = default;
    StringCharReaderState(StringCharReaderState &&) = default;
    auto operator=(const StringCharReaderState &) -> StringCharReaderState & = default;
    auto operator=(StringCharReaderState &&) -> StringCharReaderState & = default;

public:
    /// Test if two saved states are equal.
    [[nodiscard]] constexpr auto operator==(const StringCharReaderState &other) const noexcept -> bool {
        return _kind == other._kind && _storageId == other._storageId && _rawPosition == other._rawPosition &&
            _cpPosition == other._cpPosition;
    }
    /// Test if two saved states differ.
    [[nodiscard]] constexpr auto operator!=(const StringCharReaderState &other) const noexcept -> bool {
        return !operator==(other);
    }

private:
    constexpr StringCharReaderState(
        impl::StringReaderBackendKind kind,
        mem::StorageIdentifier storageId,
        std::size_t rawPosition,
        unit::CpIndex cpPosition) noexcept :
        _kind{kind}, _storageId{storageId}, _rawPosition{rawPosition}, _cpPosition{cpPosition} {}

    [[nodiscard]] constexpr auto kind() const noexcept -> impl::StringReaderBackendKind { return _kind; }
    [[nodiscard]] constexpr auto storageId() const noexcept -> mem::StorageIdentifier { return _storageId; }
    [[nodiscard]] constexpr auto rawPosition() const noexcept -> std::size_t { return _rawPosition; }
    [[nodiscard]] constexpr auto cpPosition() const noexcept -> unit::CpIndex { return _cpPosition; }

private:
    impl::StringReaderBackendKind _kind{};            ///< The backend kind this state belongs to.
    mem::StorageIdentifier _storageId;                ///< The read-only string storage identity.
    std::size_t _rawPosition{0};                      ///< The raw backend position.
    unit::CpIndex _cpPosition{unit::CpIndex::zero()}; ///< The decoded code-point position.
};

}
