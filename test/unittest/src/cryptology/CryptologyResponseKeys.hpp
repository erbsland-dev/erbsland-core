// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/Literals.hpp>

namespace response_key {

using namespace el::text::literals;

/// Response-record field `AAD`.
constexpr auto cAdditionalDataField = "AAD"_el;
/// Response-record field `CIPHERTEXT`.
constexpr auto cCiphertextField = "CIPHERTEXT"_el;
/// Response-record field `KEY`.
constexpr auto cEncryptionKeyField = "KEY"_el;
/// Response-record field `Flags`.
constexpr auto cFlagsField = "Flags"_el;
/// Response-record field `IV`.
constexpr auto cIvField = "IV"_el;
/// Response-record field `MD`.
constexpr auto cMessageDigestField = "MD"_el;
/// Response-record field `Msg`.
constexpr auto cMessageField = "Msg"_el;
/// Response-record field `PLAINTEXT`.
constexpr auto cPlaintextField = "PLAINTEXT"_el;
/// Response-record field `Result`.
constexpr auto cResultField = "Result"_el;
/// Response-record field `CT`.
constexpr auto cShortCiphertextField = "CT"_el;
/// Response-record field `PT`.
constexpr auto cShortPlaintextField = "PT"_el;
/// Response-record field `Sig`.
constexpr auto cSignatureField = "Sig"_el;
/// Response-record field `Tag`.
constexpr auto cTagField = "Tag"_el;

}
