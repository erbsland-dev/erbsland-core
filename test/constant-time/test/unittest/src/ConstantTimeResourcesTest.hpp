// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "cases/rsa/RsaContext.hpp"

#include <erbsland/Application.hpp>
#include <erbsland/Resources.hpp>
#include <erbsland/String.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace el::text::literals;

/// Verify the pinned RSA fixture is linked and usable without runtime file access.
/// @tested{ConstantTimeResourcesTest}
TESTED_TARGETS(RsaContext)
class ConstantTimeResourcesTest final : public el::UnitTest {
public:
    /// Load and parse the same embedded key used by the measurement fixtures.
    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testEmbeddedPinnedKey() {
        const auto pem =
            el::application().resources().getTextOrThrow("constant-time-fixtures"_el, "rsa-2048-pkcs8.pem"_el);
        REQUIRE(pem.contains("-----BEGIN PRIVATE KEY-----"_el));
        const auto context = app::constant_time::RsaContext{};
        REQUIRE_FALSE(context.privateKey().isEmpty());
        REQUIRE_FALSE(context.publicKey().keyData().isEmpty());
        REQUIRE(context.components().modulus.word(0) != 0);
    }
};
