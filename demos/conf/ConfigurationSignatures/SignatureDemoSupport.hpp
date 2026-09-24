// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <DemoCommon.hpp>
#include <erbsland/text/base_n/BaseNDecoder.hpp>
#include <erbsland/text/base_n/BaseNFormat.hpp>

namespace demo {

inline auto signatureDemoKey() -> el::ByteBlock {
    return el::text::base_n::BaseNDecoder{
        "WZ3RWS6xYdPNi2s8md8Gf3zkI41qfsRfIcK1QwC3u4A="_el, el::text::base_n::BaseNFormat::base64()}
        .toDataOrThrow(el::ByteLength{32U});
}

}
