// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../text/Literals.hpp"

namespace erbsland::cryptology::impl::cryptology_oids {

using namespace text::literals;

constexpr auto ed25519 = "1.3.101.112"_el;
constexpr auto ecPublicKey = "1.2.840.10045.2.1"_el;
constexpr auto secp256r1 = "1.2.840.10045.3.1.7"_el;
constexpr auto secp384r1 = "1.3.132.0.34"_el;
constexpr auto rsaEncryption = "1.2.840.113549.1.1.1"_el;
constexpr auto rsaPss = "1.2.840.113549.1.1.10"_el;

constexpr auto pbes2 = "1.2.840.113549.1.5.13"_el;
constexpr auto pbkdf2 = "1.2.840.113549.1.5.12"_el;
constexpr auto hmacSha256 = "1.2.840.113549.2.9"_el;
constexpr auto aes256Cbc = "2.16.840.1.101.3.4.1.42"_el;

constexpr auto commonName = "2.5.4.3"_el;
constexpr auto countryName = "2.5.4.6"_el;
constexpr auto localityName = "2.5.4.7"_el;
constexpr auto stateOrProvinceName = "2.5.4.8"_el;
constexpr auto organizationName = "2.5.4.10"_el;
constexpr auto organizationalUnitName = "2.5.4.11"_el;

constexpr auto subjectKeyIdentifier = "2.5.29.14"_el;
constexpr auto keyUsage = "2.5.29.15"_el;
constexpr auto subjectAlternativeName = "2.5.29.17"_el;
constexpr auto basicConstraints = "2.5.29.19"_el;
constexpr auto authorityKeyIdentifier = "2.5.29.35"_el;
constexpr auto extendedKeyUsage = "2.5.29.37"_el;
constexpr auto serverAuth = "1.3.6.1.5.5.7.3.1"_el;
constexpr auto clientAuth = "1.3.6.1.5.5.7.3.2"_el;
constexpr auto extensionRequest = "1.2.840.113549.1.9.14"_el;

}
