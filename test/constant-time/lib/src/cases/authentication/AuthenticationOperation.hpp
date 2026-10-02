// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

namespace app::constant_time {

/// Public operation selection for a fixed-shape experiment.
enum class AuthenticationOperation {
    HmacCalculate, ///< Hmac Calculate operation.
    HmacVerify,    ///< Hmac Verify operation.
    HkdfExtract,   ///< Hkdf Extract operation.
    HkdfExpand,    ///< Hkdf Expand operation.
};

}
