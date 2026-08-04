// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "KeyAgreementAlgorithm.hpp"

#include "../../mem/ByteBlock.hpp"
#include "../../mem/ByteSpan.hpp"

namespace erbsland::cryptology {

/// A public key for one generic key-agreement algorithm.
/// @seedoc{/reference/cryptology/key_agreement}
/// @tested{KeyAgreementTest}
class KeyAgreementPublicKey final {
public:
    /// Create an empty public key.
    KeyAgreementPublicKey() noexcept = default;
    /// Create a public key by copying its exact encoded bytes.
    /// @throws err::ParameterError If the algorithm or key length is invalid.
    KeyAgreementPublicKey(KeyAgreementAlgorithm algorithm, mem::ConstByteSpan data);

    // defaults
    ~KeyAgreementPublicKey() = default;
    KeyAgreementPublicKey(const KeyAgreementPublicKey &) = default;
    KeyAgreementPublicKey(KeyAgreementPublicKey &&) noexcept = default;
    auto operator=(const KeyAgreementPublicKey &) -> KeyAgreementPublicKey & = default;
    auto operator=(KeyAgreementPublicKey &&) noexcept -> KeyAgreementPublicKey & = default;

public: // tests
    /// Test whether no public key is stored.
    [[nodiscard]] auto isEmpty() const noexcept -> bool { return _data.isEmpty(); }

public: // accessors
    /// Get the key-agreement algorithm.
    [[nodiscard]] auto algorithm() const noexcept -> KeyAgreementAlgorithm { return _algorithm; }
    /// Access the owning public-key bytes.
    [[nodiscard]] auto data() const noexcept -> const mem::ByteBlock & { return _data; }
    /// Access a borrowed public-key view.
    [[nodiscard]] auto span() const noexcept -> mem::ConstByteSpan { return _data.span(); }

private:
    KeyAgreementAlgorithm _algorithm; ///< Key-agreement algorithm.
    mem::ByteBlock _data;             ///< Exact public-key bytes.
};

}
