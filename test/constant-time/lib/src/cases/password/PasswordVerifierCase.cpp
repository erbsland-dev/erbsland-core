// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "PasswordVerifierCase.hpp"

#include <erbsland/random/Random.hpp>
#include <erbsland/text/String.hpp>
#include <erbsland/unit/ByteLength.hpp>

namespace app::constant_time {

using namespace el::text::literals;

PasswordVerifierCase::PasswordVerifierCase(bool keyed) : _keyed{keyed} {
    const auto id = el::String{keyed ? "password/verifier-protected"_el : "password/verifier-compare"_el};

    configure(
        TestMetadata{
            id, "Password verifier protection and comparison after derivation"_el, "first-vs-last-invalid-verifier"_el},
        el::ByteLength{8192});
}

auto PasswordVerifierCase::create([[maybe_unused]] const bool automatic) const -> std::unique_ptr<TestCase> {
    return std::make_unique<PasswordVerifierCase>(_keyed);
}

auto PasswordVerifierCase::createFixture(el::Random &random, const bool population)
    -> std::unique_ptr<PasswordVerifierFixture> {
    return std::make_unique<PasswordVerifierFixture>(random, population, _keyed);
}

}
