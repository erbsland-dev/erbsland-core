// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/ByteBlock.hpp>
#include <erbsland/unittest/TextHelper.hpp>
#include <erbsland/unittest/UnitTest.hpp>

/// Shared binary fixture construction for the wire codecs.
class Asn1TestHelper : public el::UnitTest {
public:
    static auto bytesFromHex(const std::string_view hex) -> el::ByteBlock {
        const auto bytes = el::unittest::th::stdStringFromHex(hex);
        return el::ByteBlock::fromSpan(std::span<const char>{bytes});
    }
};
