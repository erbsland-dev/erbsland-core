// Copyright (c) 2024-2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../ConfTestHelper.hpp"

#include <erbsland/conf/ConfErrorCategory.hpp>
#include <erbsland/conf/StdFormat.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <format>
#include <stdexcept>
#include <string>

using namespace el::conf;
using namespace el::text::literals;

TESTED_TARGETS(ConfErrorCategory)
class ErrorCategoryTest final : public UNITTEST_SUBCLASS(ConfTestHelper) {

    inline static const auto cEnumValues = std::array<std::tuple<ConfErrorCategory::Enum, el::text::String>, 11>{
        std::make_tuple(ConfErrorCategory::IO, el::text::String{"IO"_el}),
        std::make_tuple(ConfErrorCategory::Encoding, el::text::String{"Encoding"_el}),
        std::make_tuple(ConfErrorCategory::UnexpectedEnd, el::text::String{"UnexpectedEnd"_el}),
        std::make_tuple(ConfErrorCategory::Character, el::text::String{"Character"_el}),
        std::make_tuple(ConfErrorCategory::Syntax, el::text::String{"Syntax"_el}),
        std::make_tuple(ConfErrorCategory::LimitExceeded, el::text::String{"LimitExceeded"_el}),
        std::make_tuple(ConfErrorCategory::NameConflict, el::text::String{"NameConflict"_el}),
        std::make_tuple(ConfErrorCategory::Indentation, el::text::String{"Indentation"_el}),
        std::make_tuple(ConfErrorCategory::Unsupported, el::text::String{"Unsupported"_el}),
        std::make_tuple(ConfErrorCategory::Signature, el::text::String{"Signature"_el}),
        std::make_tuple(ConfErrorCategory::Internal, el::text::String{"Internal"_el})};

    ConfErrorCategory ec{};

public:
    auto additionalErrorMessages() -> std::string override {
        try {
            return std::format(
                "ConfErrorCategory: enum={}, text={}, code={}",
                static_cast<int>(static_cast<ConfErrorCategory::Enum>(ec)),
                el::text::StringConverter{ec.toText()}.toStdString(),
                ec.toCode());
        } catch (...) {
            return {"ConfErrorCategory: unknown error"};
        }
    }

    void setUp() override { ec = {}; }

    void testDefaultConstruction() {
        ConfErrorCategory const ec;
        REQUIRE(ec == ConfErrorCategory::Internal);
    }

    void testConstructor() {
        for (const auto &[value, text] : cEnumValues) {
            ConfErrorCategory const ec{value};
            REQUIRE(ec == value);
            REQUIRE(ec == ConfErrorCategory{value});
        }
    }

    void testEnumAssignment() {
        ec = ConfErrorCategory::IO;
        REQUIRE(ec == ConfErrorCategory::IO);

        ec = ConfErrorCategory::Encoding;
        REQUIRE(ec == ConfErrorCategory::Encoding);
    }

    void testToText() {
        for (const auto &[value, text] : cEnumValues) {
            ec = value;
            REQUIRE(ec.toText() == text);
        }
    }

    void testToCode() {
        ec = ConfErrorCategory::IO;
        REQUIRE(ec.toCode() == 1);

        ec = ConfErrorCategory::Signature;
        REQUIRE(ec.toCode() == 10);
    }

    void testCopyAndAssignment() {
        const ConfErrorCategory ec1 = ConfErrorCategory::UnexpectedEnd;
        const ConfErrorCategory ec2 = ec1; // Copy constructor
        REQUIRE(ec1 == ec2);

        ConfErrorCategory ec3;
        ec3 = ec1; // Assignment operator
        REQUIRE(ec3 == ec1);
    }

    void testCast() {
        ec = ConfErrorCategory::UnexpectedEnd;
        REQUIRE(static_cast<ConfErrorCategory::Enum>(ec) == ConfErrorCategory::UnexpectedEnd);
        REQUIRE(static_cast<int>(ec) == 3);
    }

    void testOperators() {
        WITH_CONTEXT(
            requireAllOperators<ConfErrorCategory, ConfErrorCategory>(
                ConfErrorCategory{ConfErrorCategory::IO},
                ConfErrorCategory{ConfErrorCategory::Encoding},
                ConfErrorCategory{ConfErrorCategory::Internal},
                ConfErrorCategory{ConfErrorCategory::IO},
                ConfErrorCategory{ConfErrorCategory::Encoding},
                ConfErrorCategory{ConfErrorCategory::Internal}));
        WITH_CONTEXT(
            requireAllOperators<ConfErrorCategory, ConfErrorCategory::Enum>(
                ConfErrorCategory{ConfErrorCategory::IO},
                ConfErrorCategory{ConfErrorCategory::Encoding},
                ConfErrorCategory{ConfErrorCategory::Internal},
                ConfErrorCategory::IO,
                ConfErrorCategory::Encoding,
                ConfErrorCategory::Internal));
        WITH_CONTEXT(
            requireAllOperators<ConfErrorCategory::Enum, ConfErrorCategory>(
                ConfErrorCategory::IO,
                ConfErrorCategory::Encoding,
                ConfErrorCategory::Internal,
                ConfErrorCategory{ConfErrorCategory::IO},
                ConfErrorCategory{ConfErrorCategory::Encoding},
                ConfErrorCategory{ConfErrorCategory::Internal}));
    }
};
