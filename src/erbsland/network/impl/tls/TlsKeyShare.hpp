#pragma once

#include "../../../cryptology/keys/KeyAgreementAlgorithm.hpp"
#include "../../../mem/ByteBlock.hpp"

#include <cstdint>
#include <optional>
#include <vector>

/// TLS 1.3 named-group mapping and configuration validation (RFC 8446 sections 4.2.7 and 4.2.8).
namespace erbsland::network::impl::tls_key_share {

/// Map a supported agreement algorithm to its TLS NamedGroup.
/// @param algorithm Supported agreement algorithm.
/// @return The TLS wire value.
/// @tested{TlsClientProtocolTest TlsServerProtocolTest}
[[nodiscard]] auto group(cryptology::KeyAgreementAlgorithm algorithm) -> uint16_t;
/// Decode a supported NamedGroup.
/// @param group TLS wire value.
/// @return The agreement algorithm, or no value for an unsupported group.
/// @tested{TlsClientProtocolTest TlsServerProtocolTest}
[[nodiscard]] auto algorithm(uint16_t group) noexcept -> std::optional<cryptology::KeyAgreementAlgorithm>;
/// Validate a nonempty, duplicate-free preference list of supported groups.
/// @param groups Ordered agreement algorithms.
/// @tested{TlsClientProtocolTest TlsServerProtocolTest}
void validate(const std::vector<cryptology::KeyAgreementAlgorithm> &groups);
/// Get the fixed HelloRetryRequest random from RFC 8446 section 4.1.3.
/// @tested{TlsClientProtocolTest TlsServerProtocolTest}
[[nodiscard]] auto helloRetryRequestRandom() -> mem::ByteBlock;

}
