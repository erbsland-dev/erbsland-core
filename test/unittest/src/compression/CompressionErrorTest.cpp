// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/compression/CompressionError.hpp>
#include <erbsland/compression/CompressionErrorContext.hpp>
#include <erbsland/err/RuntimeError.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/TextDocument.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <string>
#include <type_traits>

using namespace el::text::literals;

TESTED_TARGETS(CompressionError CompressionErrorContext CompressionErrorDiagnostic)
TAGS(Errors)
class CompressionErrorTest final : public el::UnitTest {
public:
    void testContextAndConvenienceAccessors() {
        auto context = el::compression::CompressionErrorContext{
            el::compression::CompressionErrorReason::UnsupportedFeature,
            "Compressed data uses an unsupported feature"_el,
            "The selected codec profile cannot decode this representation."_el};
        const auto error = el::compression::CompressionError{context};

        static_assert(std::is_base_of_v<el::err::RuntimeError, el::compression::CompressionError>);
        REQUIRE(error.reasonCode() == el::compression::CompressionErrorReason::UnsupportedFeature);
        REQUIRE_EQUAL(error.title(), "Compressed data uses an unsupported feature"_el);
        REQUIRE_EQUAL(error.description(), "The selected codec profile cannot decode this representation."_el);
        REQUIRE_EQUAL(error.context().reason(), context.reason());
        REQUIRE_EQUAL(error.reason(), error.title());
        REQUIRE_EQUAL(std::string{error.what()}, std::string{"Compressed data uses an unsupported feature"});

        context.setReason(el::compression::CompressionErrorReason::LengthMismatch)
            .setTitle("Compressed length is invalid"_el)
            .setDescription("The declared and decoded lengths differ."_el);
        REQUIRE(context.reason() == el::compression::CompressionErrorReason::LengthMismatch);
        REQUIRE_EQUAL(context.title(), "Compressed length is invalid"_el);
        REQUIRE_EQUAL(context.description(), "The declared and decoded lengths differ."_el);
    }

    void testDiagnosticContainsHumanReadableDetails() {
        const auto error = el::compression::CompressionError{el::compression::CompressionErrorContext{
            el::compression::CompressionErrorReason::LengthMismatch,
            "Compressed length is invalid"_el,
            "The declared and decoded lengths differ."_el}};
        const auto diagnostic = error.diagnostic();

        REQUIRE(diagnostic);
        const auto text = diagnostic->toString();
        REQUIRE(text.contains("Compressed length is invalid"_el));
        REQUIRE(text.contains("The declared and decoded lengths differ."_el));
        REQUIRE(text.contains("reason"_el));
        REQUIRE(text.contains("Length mismatch"_el));
    }

    void testLegacyConstructorCreatesContext() {
        const auto error = el::compression::CompressionError{
            el::compression::CompressionErrorReason::MalformedData, "Compressed payload is malformed."};

        REQUIRE(error.reasonCode() == el::compression::CompressionErrorReason::MalformedData);
        REQUIRE_EQUAL(error.title(), "Compressed payload is malformed."_el);
        REQUIRE(error.description().isEmpty());
        REQUIRE(error.diagnostic()->toTextDocument().toString().contains("Malformed data"_el));
    }
};
