// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/err/RuntimeError.hpp>
#include <erbsland/MakeOneNamespace.hpp>
#include <erbsland/mem/ByteBlock.hpp>
#include <erbsland/mem/ByteBlockEditor.hpp>
#include <erbsland/text/IntegerBase.hpp>
#include <erbsland/text/String.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/unittest/TextHelper.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <span>
#include <string_view>

/// Shared fixtures and conversion helpers for cryptology tests.
/// @notest{Shared unit-test helper.}
class CryptologyTestHelper : public el::UnitTest {
public:
    /// Convert compact hexadecimal Core text into a byte block.
    [[nodiscard]] static auto bytesFromHex(const el::String &hex) -> el::ByteBlock {
        using namespace el::text::literals;

        auto result = el::ByteBlockEditor{};
        result.reserve(el::ByteLength{hex.length().toSizeT() / 2U});
        auto index = el::ByteIndex::zero();
        while (index.toSizeT() < hex.length().toSizeT()) {
            const auto high = hex.readCharAndAdvance(index).digitValue(el::IntegerBase::Hexadecimal);
            const auto low = hex.readCharAndAdvance(index).digitValue(el::IntegerBase::Hexadecimal);
            if (!high.has_value() || !low.has_value()) {
                throw el::RuntimeError{"Invalid compact hexadecimal test data."_el};
            }
            result.append(el::Byte{static_cast<uint8_t>((*high << 4U) | *low)});
        }
        return result;
    }

    /// Convert compact hexadecimal test data into a byte block.
    [[nodiscard]] static auto bytesFromHex(const std::string_view hex) -> el::ByteBlock {
        const auto bytes = el::unittest::th::stdStringFromHex(hex);
        return el::ByteBlock::fromSpan(std::span<const char>{bytes});
    }
};
