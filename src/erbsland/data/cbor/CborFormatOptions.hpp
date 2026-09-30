// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once
namespace erbsland::data::cbor {
/// Profile for serializing CBOR.
/// @tested{CborValueTest}
class CborFormatOptions final {
public:
    /// Test whether strict DAG-CBOR output is selected.
    [[nodiscard]] constexpr auto isDagCbor() const noexcept -> bool { return _dagCbor; }
    /// Select strict DAG-CBOR output.
    constexpr auto setDagCbor(bool value) noexcept -> CborFormatOptions & {
        _dagCbor = value;
        return *this;
    }

private:
    bool _dagCbor{false}; ///< Strict DAG-CBOR profile.
};
}
