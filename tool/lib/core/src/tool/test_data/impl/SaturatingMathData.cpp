// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "SaturatingMathData.hpp"

#include <erbsland/LogicFailure.hpp>
#include <erbsland/StringFormat.hpp>

#include <array>

namespace tool::test_data::impl {

using namespace el::text::literals;

SaturatingMathData::SaturatingMathData() {
    // The boundary catalog is deliberately independent of SaturatingMath and native arithmetic overflow.
    constexpr auto boundaries = std::array<uint64_t, 32>{
        0,
        1,
        2,
        9,
        23,
        64,
        126,
        127,
        128,
        129,
        254,
        255,
        256,
        32766,
        32767,
        32768,
        32769,
        65534,
        65535,
        65536,
        2147483646,
        2147483647,
        2147483648,
        2147483649,
        4294967294,
        4294967295,
        4294967296,
        9223372036854775806ULL,
        9223372036854775807ULL,
        9223372036854775808ULL,
        18446744073709551614ULL,
        18446744073709551615ULL};

    for (const auto bits : {8, 16, 32, 64}) {
        for (const auto isSigned : {true, false}) {
            auto sample = Samples{};
            auto limit = el::BigInteger{1};
            auto exponent = bits;
            if (isSigned) {
                --exponent;
            }
            for (auto index = 0; index < exponent; ++index) {
                limit *= el::BigInteger{2};
            }
            auto prefix = "u"_el;
            if (isSigned) {
                sample.minimum = -limit;
                prefix = "i"_el;
            }
            sample.maximum = limit - el::BigInteger{1};
            sample.name = el::StringFormat{"{}{}"_el}.build(prefix, bits);
            auto candidates = el::List<el::BigInteger>{
                sample.minimum, sample.minimum + el::BigInteger{1}, sample.maximum - el::BigInteger{1}, sample.maximum};
            for (const auto boundary : boundaries) {
                candidates.append(el::BigInteger{boundary});
                if (isSigned) {
                    candidates.append(-el::BigInteger{boundary});
                }
            }
            candidates.sort();
            for (const auto &value : candidates) {
                if (value < sample.minimum || value > sample.maximum ||
                    (!sample.values.isEmpty() && sample.values.last() == value)) {
                    continue;
                }
                sample.values.append(value);
                sample.decimals.append(value.toString());
            }
            _types.append(std::move(sample));
        }
    }
}

auto SaturatingMathData::Samples::clamp(const el::BigInteger &value) const -> el::BigInteger {
    if (value < minimum) {
        return minimum;
    }
    if (value > maximum) {
        return maximum;
    }
    return value;
}

auto SaturatingMathData::name(const Operation operation) -> el::String {
    switch (operation) {
    case Operation::Cast:
        return "cast"_el;
    case Operation::Add:
        return "add"_el;
    case Operation::Subtract:
        return "subtract"_el;
    case Operation::Multiply:
        return "multiply"_el;
    case Operation::Divide:
        return "divide"_el;
    case Operation::Modulo:
        return "modulo"_el;
    }
    throw el::LogicFailure{"Unknown fixture operation."_el};
}

auto SaturatingMathData::calculate(const Operation operation, const el::BigInteger &first, const el::BigInteger &second)
    -> el::BigInteger {
    switch (operation) {
    case Operation::Add:
        return first + second;
    case Operation::Subtract:
        return first - second;
    case Operation::Multiply:
        return first * second;
    case Operation::Divide:
        return first / second;
    case Operation::Modulo:
        return first % second;
    default:
        throw el::LogicFailure{"Expected a binary fixture operation."_el};
    }
}

auto SaturatingMathData::rows(const Operation operation) const -> el::render::ValueList {
    auto rows = el::render::ValueList{};
    for (const auto &target : _types) {
        for (const auto &source : _types) {
            if (operation == Operation::Cast) {
                for (auto index = el::ItemIndex{}; index.isWithin(source.values.count()); ++index) {
                    const auto &value = source.values.getRef(index);
                    const auto expected = target.clamp(value);
                    rows.append(
                        el::render::Value{el::render::ValueList{
                            target.name,
                            source.name,
                            source.decimals.get(index),
                            expected.toString(),
                            static_cast<int>(expected != value)}});
                }
                continue;
            }
            for (auto firstIndex = el::ItemIndex{}; firstIndex.isWithin(target.values.count()); ++firstIndex) {
                const auto &first = target.values.getRef(firstIndex);
                for (auto secondIndex = el::ItemIndex{}; secondIndex.isWithin(source.values.count()); ++secondIndex) {
                    const auto &second = source.values.getRef(secondIndex);
                    if ((operation == Operation::Divide || operation == Operation::Modulo) && second.isZero()) {
                        continue;
                    }
                    const auto result = calculate(operation, first, second);
                    const auto expected = target.clamp(result);
                    const auto overflow = operation != Operation::Modulo && expected != result;
                    rows.append(
                        el::render::Value{el::render::ValueList{
                            target.name,
                            source.name,
                            target.decimals.get(firstIndex),
                            source.decimals.get(secondIndex),
                            expected.toString(),
                            static_cast<int>(overflow)}});
                }
            }
        }
    }
    return rows;
}

}
