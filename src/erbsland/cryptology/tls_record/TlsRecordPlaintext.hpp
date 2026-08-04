// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TlsRecordContentType.hpp"

#include "../../mem/ByteBlock.hpp"

namespace erbsland::cryptology {

/// Authenticated content recovered from one TLS 1.3 record.
/// The content type is the final nonzero TLSInnerPlaintext octet from RFC 8446 section 5.2. Content is retained in
/// sensitive storage and is never constructed until AEAD authentication succeeds.
/// @seedoc{/reference/cryptology/tls_record_protection}
/// @tested{TlsRecordProtectionTest}
class TlsRecordPlaintext final {
public:
    /// Create authenticated record content.
    TlsRecordPlaintext(TlsRecordContentType type, mem::ByteBlock content) noexcept;
    /// Securely erase retained plaintext.
    ~TlsRecordPlaintext();

    // defaults/deletions
    TlsRecordPlaintext(const TlsRecordPlaintext &) = delete;
    TlsRecordPlaintext(TlsRecordPlaintext &&) noexcept = default;
    auto operator=(const TlsRecordPlaintext &) -> TlsRecordPlaintext & = delete;
    auto operator=(TlsRecordPlaintext &&) noexcept -> TlsRecordPlaintext & = default;

public:
    /// Securely erase content and restore an empty payload.
    void secureErase() noexcept;
    /// Transfer the authenticated content to the caller.
    [[nodiscard]] auto takeContent() noexcept -> mem::ByteBlock;

public: // tests
    /// Test whether the authenticated content is empty.
    [[nodiscard]] auto isEmpty() const noexcept -> bool { return _content.isEmpty(); }

public: // accessors
    /// Get the authenticated inner content type.
    [[nodiscard]] auto type() const noexcept -> TlsRecordContentType { return _type; }
    /// Borrow the authenticated content.
    [[nodiscard]] auto content() const noexcept -> const mem::ByteBlock & { return _content; }

private:
    TlsRecordContentType _type; ///< Authenticated inner content type.
    mem::ByteBlock _content;    ///< Authenticated sensitive content.
};

}
