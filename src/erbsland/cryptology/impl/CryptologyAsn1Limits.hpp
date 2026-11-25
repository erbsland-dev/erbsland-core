// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../data/asn1/Asn1Limits.hpp"

namespace erbsland::cryptology::impl {

/// Resource budgets for certificate, key, and signature input.
inline constexpr auto cCryptologyAsn1Limits = data::asn1::Asn1Limits{.maximumLength = unit::ByteCount{1024U * 1024U}};

}
