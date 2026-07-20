// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../ConfTestHelper.hpp"

#include <erbsland/conf/impl/assignment/AssignmentStream.hpp>
#include <erbsland/conf/impl/lexer/Lexer.hpp>
#include <erbsland/conf/impl/utilities/InternalView.hpp>
#include <erbsland/conf/impl/value/Value.hpp>
#include <erbsland/conf/StdFormatForConf.hpp>

#include <cmath>
#include <filesystem>
#include <format>
#include <sstream>
#include <utility>

using namespace el::conf;
using impl::Assignment;
using impl::AssignmentGenerator;
using impl::AssignmentStream;
using impl::AssignmentStreamPtr;
using impl::AssignmentType;
using impl::CharStream;
using impl::InternalView;
using impl::Lexer;
using impl::LexerPtr;
using std::filesystem::path;
using namespace el::text::literals;

class AssignmentStreamHelper : public ConfTestHelper {
public:
    path testFilePath;
    SourcePtr source;
    LexerPtr lexer;
    AssignmentStreamPtr stream;
    AssignmentGenerator generator;
    Assignment assignment;

    auto additionalErrorMessages() -> std::string override {
        try {
            std::ostringstream oss;
            oss << "path: " << testFilePath << "\n";
            if (lexer != nullptr) {
                oss << "lexer:\n" << el::text::StringConverter{internalView(*lexer)->toString(2)}.toStdString() << "\n";
            }
            return oss.str();
        } catch (...) {
            return "Unexpected exception thrown";
        }
    }

    void setupAssignmentStream(const std::string &fileName) {
        using std::filesystem::path;
        testFilePath = path(unitTestExecutablePath()).parent_path() / "data" / "conf" / "assignment_stream" / fileName;
        source = Source::fromFile(el::path::Path{testFilePath});
        REQUIRE_NOTHROW(source->open());
        lexer = Lexer::create(CharStream::create(source));
        stream = AssignmentStream::create(lexer);
        generator = stream->assignments();
    }

    void requireAssignment() {
        auto nextAssignment = generator.next();
        REQUIRE(nextAssignment.has_value());
        assignment = std::move(*nextAssignment);
    }

    void requireValue(const el::text::String &expectedNamePath, const ValueType expectedValueType) {

        requireAssignment();
        REQUIRE_EQUAL(assignment.namePath().toText(), expectedNamePath);
        REQUIRE_EQUAL(assignment.type(), AssignmentType::Value);
        REQUIRE_EQUAL(assignment.value()->type(), expectedValueType);
    }

    template <typename T>
    void requireValue(
        const el::text::String &expectedNamePath, const ValueType expectedValueType, const T &expectedValue) {

        requireAssignment();
        REQUIRE_EQUAL(assignment.namePath().toText(), expectedNamePath);
        REQUIRE_EQUAL(assignment.type(), AssignmentType::Value);
        REQUIRE_EQUAL(assignment.value()->type(), expectedValueType);
        if constexpr (std::is_same_v<T, el::re::RegExPtr>) {
            const auto actualValue = assignment.value()->asType<T>();
            REQUIRE(actualValue != nullptr);
            REQUIRE(expectedValue != nullptr);
            REQUIRE_EQUAL(actualValue->pattern(), expectedValue->pattern());
        } else {
            REQUIRE_EQUAL(assignment.value()->asType<T>(), expectedValue);
        }
    }

    auto compareFloat(double actual, double expected) {
        if (std::isnan(expected)) {
            REQUIRE(std::isnan(actual));
        } else if (std::isinf(expected)) {
            REQUIRE_EQUAL(actual, expected);
        } else {
            const auto delta = std::abs(actual - expected);
            REQUIRE(delta < std::numeric_limits<double>::epsilon());
        }
    }

    void requireFloat(const el::text::String &expectedNamePath, double expectedValue) {

        requireAssignment();
        REQUIRE_EQUAL(assignment.namePath().toText(), expectedNamePath);
        REQUIRE_EQUAL(assignment.type(), AssignmentType::Value);
        REQUIRE_EQUAL(assignment.value()->type(), ValueType::Float);
        auto actualValue = assignment.value()->asFloat();
        runWithContext(
            SOURCE_LOCATION(),
            [&]() { compareFloat(actualValue, expectedValue); },
            [&]() -> std::string {
                return std::format("Failed comparing floats: {} == {}", actualValue, expectedValue);
            });
    }

    struct ExpectedListEntry {
        ValueType type;
        impl::Content content;
    };

    void requireList(const el::text::String &expectedNamePath, const std::vector<ExpectedListEntry> &expectedList) {

        // sanity checks for the unit test
        REQUIRE(expectedList.size() > 1);

        requireAssignment();
        REQUIRE_EQUAL(assignment.namePath().toText(), expectedNamePath);
        REQUIRE_EQUAL(assignment.type(), AssignmentType::Value);
        REQUIRE_EQUAL(assignment.value()->type(), ValueType::ValueList);
        const auto valueList = assignment.value()->asValueList();
        REQUIRE_EQUAL(valueList.size(), expectedList.size());
        for (std::size_t i = 0; i < valueList.size(); ++i) {
            const auto &value = valueList[i];
            const auto &expectedType = expectedList[i].type;
            const auto &expectedContent = expectedList[i].content;
            runWithContext(
                SOURCE_LOCATION(),
                [&]() {
                    REQUIRE_EQUAL(value->type(), expectedType);
                    std::visit(
                        [&]<typename T>(const T &expectedValue) -> void {
                            if constexpr (std::is_same_v<T, impl::NoContent>) {
                                // ignore no content
                            } else if constexpr (std::is_same_v<T, el::re::RegExPtr>) {
                                const auto actualValue = value->asType<T>();
                                REQUIRE(actualValue != nullptr);
                                REQUIRE(expectedValue != nullptr);
                                REQUIRE_EQUAL(actualValue->pattern(), expectedValue->pattern());
                            } else {
                                const auto actualValue = value->asType<T>();
                                REQUIRE_EQUAL(actualValue, expectedValue);
                            }
                        },
                        expectedContent);
                },
                [&]() -> std::string { return std::format("Failed for list index: {}", i); });
        }
    }

    template <typename T>
    void requireMetaValue(
        const el::text::String &expectedNamePath, const ValueType expectedValueType, const T &expectedValue) {

        requireAssignment();
        REQUIRE_EQUAL(assignment.namePath().toText(), expectedNamePath);
        REQUIRE_EQUAL(assignment.type(), AssignmentType::MetaValue);
        REQUIRE_EQUAL(assignment.value()->type(), expectedValueType);
        if constexpr (std::is_same_v<T, el::re::RegExPtr>) {
            const auto actualValue = assignment.value()->asType<T>();
            REQUIRE(actualValue != nullptr);
            REQUIRE(expectedValue != nullptr);
            REQUIRE_EQUAL(actualValue->pattern(), expectedValue->pattern());
        } else {
            REQUIRE_EQUAL(assignment.value()->asType<T>(), expectedValue);
        }
    }

    void requireSectionMap(const el::text::String &expectedNamePath) {

        requireAssignment();
        REQUIRE_EQUAL(assignment.namePath().toText(), expectedNamePath);
        REQUIRE_EQUAL(assignment.type(), AssignmentType::SectionMap);
        REQUIRE(assignment.value() == nullptr);
    }

    void requireSectionList(const el::text::String &expectedNamePath) {

        requireAssignment();
        REQUIRE_EQUAL(assignment.namePath().toText(), expectedNamePath);
        REQUIRE_EQUAL(assignment.type(), AssignmentType::SectionList);
        REQUIRE(assignment.value() == nullptr);
    }

    void requireEnd() {
        requireAssignment();
        REQUIRE_EQUAL(assignment.type(), AssignmentType::EndOfDocument);
        REQUIRE_FALSE(generator.next().has_value());
    }
};
