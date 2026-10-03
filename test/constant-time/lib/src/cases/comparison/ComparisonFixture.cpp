// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ComparisonFixture.hpp"

#include <erbsland/Byte.hpp>
#include <erbsland/ByteArray.hpp>
#include <erbsland/ByteBlock.hpp>
#include <erbsland/ByteBuffer.hpp>
#include <erbsland/ByteCount.hpp>
#include <erbsland/ByteIndex.hpp>
#include <erbsland/LogicFailure.hpp>
#include <erbsland/Random.hpp>
#include <erbsland/String.hpp>

namespace app::constant_time {

using namespace el::text::literals;

ComparisonFixture::ComparisonFixture(
    el::Random &random, const bool population, el::ByteCount size, ComparisonMode mode, ComparisonContainer container) :
    _container{container} {

    auto left = input(random, mode == ComparisonMode::FixedRandom && population, size);
    if (mode == ComparisonMode::Equal) {
        left.fill(population ? el::Byte{} : el::Byte{0x55U});
    }
    // Distinct storage exercises the comparison even if the container later gains an alias fast path.
    auto right = el::ByteBuffer{left.span()};
    require(left.span().data() != right.span().data());
    if (mode == ComparisonMode::MismatchPosition) {
        right.set(
            el::ByteIndex::end(population ? size - el::ByteCount::one() : el::ByteCount::zero()), el::Byte{0x54U});
    }
    _left = std::move(left);
    _right = std::move(right);
    _leftBlock = el::ByteBlock::fromSpan(_left.span());
    _rightBlock = el::ByteBlock::fromSpan(_right.span());
    if (size == el::ByteCount{32U}) {
        _leftArray = el::ByteArray<32>::fromSpanOrThrow(_left.span());
        _rightArray = el::ByteArray<32>::fromSpanOrThrow(_right.span());
    }
    require(_left.isEqualConstTime(_right.span()) == (mode != ComparisonMode::MismatchPosition));
}

template <ComparisonContainer tOperation>
auto ComparisonFixture::sample() -> uint64_t {
    constexpr auto container = tOperation;

    if constexpr (container == ComparisonContainer::Buffer) {
        return _left.isEqualConstTime(_right.span());
    } else if constexpr (container == ComparisonContainer::Block) {
        return _leftBlock.isEqualConstTime(_rightBlock.span());
    } else {
        return _leftArray.isEqualConstTime(_rightArray.span());
    }
}

auto ComparisonFixture::measure(const uint64_t repetitions) -> uint64_t {
    switch (_container) {
    case ComparisonContainer::Buffer:
        return repeat(repetitions, [this]() -> uint64_t { return sample<ComparisonContainer::Buffer>(); });
    case ComparisonContainer::Block:
        return repeat(repetitions, [this]() -> uint64_t { return sample<ComparisonContainer::Block>(); });
    case ComparisonContainer::Array:
        return repeat(repetitions, [this]() -> uint64_t { return sample<ComparisonContainer::Array>(); });
    }
    throw el::LogicFailure{"Unknown sample operation."_el};
}

}
