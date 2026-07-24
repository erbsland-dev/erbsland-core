// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/mem/ByteBlock.hpp>
#include <erbsland/unittest/TextHelper.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <span>
#include <string_view>

class CryptologyTestHelper : public el::UnitTest {
public:
    /// Convert compact hexadecimal test data into a byte block.
    [[nodiscard]] static auto bytesFromHex(const std::string_view hex) -> el::mem::ByteBlock {
        const auto bytes = el::unittest::th::stdStringFromHex(hex);
        return el::mem::ByteBlock::fromSpan(std::span<const char>{bytes});
    }
};
