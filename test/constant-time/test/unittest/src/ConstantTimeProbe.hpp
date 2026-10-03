// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TestCase.hpp"

#include <erbsland/ByteCount.hpp>
#include <erbsland/Definitions.hpp>
#include <erbsland/ItemIndex.hpp>
#include <erbsland/List.hpp>
#include <erbsland/LogicFailure.hpp>
#include <erbsland/Random.hpp>
#include <erbsland/String.hpp>
#include <erbsland/TimeDelta.hpp>
#include <erbsland/TimePoint.hpp>

#include <exception>
#include <limits>

using namespace el::text::literals;

/// Deterministic fixture advancing an injected clock instead of sleeping.
/// @notest{Unit-test support.}
class ConstantTimeProbe final : public app::constant_time::TestCase {
public:
    el::TimePoint now;                               ///< Injected monotonic clock.
    uint64_t prepared{};                             ///< Current fixture count.
    uint64_t calls{};                                ///< Execution count.
    el::ByteCount bytes{1024};                       ///< Injectable memory estimate.
    el::TimeDelta prepareDelay{el::Microseconds{1}}; ///< Synthetic cost controlling the pilot batch size.
    bool capture{};           ///< Record fixture bytes and population labels for deterministic reproduction checks.
    el::List<uint64_t> trace; ///< Executed fixture values, with the population encoded in the low bit.
    std::exception_ptr preparationFailure; ///< Exception injected during preparation.
    app::constant_time::TestMetadata description{"probe"_el, "Deterministic probe"_el}; ///< Description.
    [[nodiscard]] auto metadata() const noexcept -> const app::constant_time::TestMetadata & override {
        return description;
    }
    [[nodiscard]] auto fixtureBytes() const noexcept -> el::ByteCount override { return bytes; }
    void clear() override {
        prepared = 0;
        _values.clear();
    }
    void prepare(el::Random &random, const bool population) override {
        if (preparationFailure) {
            std::rethrow_exception(preparationFailure);
        }
        if (capture) {
            const auto value = random.getUInt64(0, std::numeric_limits<uint64_t>::max());
            _values.append((value & ~uint64_t{1}) | static_cast<uint64_t>(population));
        }
        ++prepared;
        now += prepareDelay;
    }
    [[nodiscard]] auto execute(el::ItemIndex index, uint64_t repetitions) -> uint64_t override {
        if (index.toRawValue() >= prepared) {
            throw el::LogicFailure{"Unprepared fixture"_el};
        }
        if (capture) {
            trace.append(_values.getRefOrThrow(index));
        }
        ++calls;
        now += el::TimeDelta{el::Microseconds{20}} * el::TimeDelta::IntegerValue{repetitions};
        return calls;
    }

private:
    el::List<uint64_t> _values; ///< Current prepared fixture values.
};
