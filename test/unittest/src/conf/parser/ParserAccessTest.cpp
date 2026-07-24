// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ParserTestHelper.hpp"

#include <erbsland/conf/StdFormat.hpp>

using namespace el::text::literals;

TESTED_TARGETS(Parser FileAccessCheck)
class ParserAccessTest final : public UNITTEST_SUBCLASS(ParserTestHelper) {
public:
    void testInvalidFileAccessFeaturesAreParameterErrors() {
        auto accessCheck = FileAccessCheck::create();
        const auto invalidFeature = static_cast<FileAccessCheck::Feature>(FileAccessCheck::_featureCount);
        REQUIRE_THROWS_AS(el::err::ParameterError, accessCheck->enable(invalidFeature));
        REQUIRE_THROWS_AS(el::err::ParameterError, accessCheck->disable(invalidFeature));
        REQUIRE_THROWS_AS(el::err::ParameterError, accessCheck->isEnabled(invalidFeature));
    }

    void tearDown() override {
        cleanUpTestFileDirectory();
        doc = {};
    }

    void expectParserError(
        const std::filesystem::path &path,
        const ConfErrorCategory errorCategory,
        el::text::String expectedWordInErrorMessage = {}) {
        try {
            Parser parser;
            doc = parser.parseOrThrow(Source::fromFile(el::path::Path{path}));
            REQUIRE(false);
        } catch (const ConfError &error) {
            REQUIRE_EQUAL(error.category(), errorCategory);
            if (!expectedWordInErrorMessage.isEmpty()) {
                const auto success = error.description().contains(expectedWordInErrorMessage);
                if (!success) {
                    consoleWriteLine(el::text::StringConverter{error.toString()}.toStdString());
                }
                REQUIRE(success);
            }
        }
    }

    enum class TestFile {
        SameDirectory,
        Subdirectory,
        ParentDirectory,
        WrongSuffix,
    };

    struct TestFileData {
        std::filesystem::path path;
        el::text::String includeText;
    };

    std::map<TestFile, TestFileData> testFileData{
        {TestFile::SameDirectory, {"config/file.elcl", "file.elcl"_el}},
        {TestFile::Subdirectory, {"config/sub/file.elcl", "sub/file.elcl"_el}},
        {TestFile::ParentDirectory, {"file.elcl", "../file.elcl"_el}},
        {TestFile::WrongSuffix, {"config/file.txt", "file.txt"_el}},
    };

    struct TestData {
        TestFile testFile;
        std::vector<FileAccessCheck::Feature> enabledFeatures;
        std::vector<FileAccessCheck::Feature> disabledFeatures;
        bool expectedAccessGranted;
    };

    void verifyAccess(const TestData &data) {
        // Prepare the test environment.
        auto document = el::text::StringEditor{"[main]\nvalue: 123\n@include: \""_el};
        const auto testFile = testFileData.at(data.testFile);
        document.append(testFile.includeText);
        document.append("\"\n# end\n"_el);
        auto mainFile = createTestFile("config/main.elcl", el::text::String{document});
        createTestFile(testFile.path, "[other]\nvalue: 456\n"_el);
        // Create a custom access check.
        auto accessCheck = FileAccessCheck::create();
        for (auto feature : data.enabledFeatures) {
            accessCheck->enable(feature);
        }
        for (auto feature : data.disabledFeatures) {
            accessCheck->disable(feature);
        }
        // Set up the parser
        Parser parser;
        parser.setAccessCheck(accessCheck);
        auto source = Source::fromFile(el::path::Path{mainFile});
        if (data.expectedAccessGranted) {
            REQUIRE_NOTHROW(doc = parser.parseOrThrow(source));
        } else {
            try {
                doc = parser.parseOrThrow(source);
                REQUIRE(false);
            } catch (const ConfError &error) {
                REQUIRE_EQUAL(error.category(), ConfErrorCategory::Access);
            }
        }
    }

    void testIncludeGranted() {
        const auto testData = std::vector<TestData>{
            // Establish the baseline, with all default settings.
            {TestFile::ParentDirectory, {}, {}, false}, // 0
            {TestFile::SameDirectory, {}, {}, true},
            {TestFile::Subdirectory, {}, {}, true},
            {TestFile::WrongSuffix, {}, {}, true},
            //
            {TestFile::ParentDirectory, {}, {FileAccessCheck::SameDirectory}, false}, // 4
            {TestFile::SameDirectory, {}, {FileAccessCheck::SameDirectory}, false},
            {TestFile::Subdirectory, {}, {FileAccessCheck::SameDirectory}, true},
            {TestFile::WrongSuffix, {}, {FileAccessCheck::SameDirectory}, false},
            //
            {TestFile::ParentDirectory, {}, {FileAccessCheck::Subdirectories}, false}, // 8
            {TestFile::SameDirectory, {}, {FileAccessCheck::Subdirectories}, true},
            {TestFile::Subdirectory, {}, {FileAccessCheck::Subdirectories}, false},
            {TestFile::WrongSuffix, {}, {FileAccessCheck::Subdirectories}, true},
            //
            {TestFile::ParentDirectory,
                {},
                {FileAccessCheck::SameDirectory, FileAccessCheck::Subdirectories},
                false}, // 12
            {TestFile::SameDirectory, {}, {FileAccessCheck::SameDirectory, FileAccessCheck::Subdirectories}, false},
            {TestFile::Subdirectory, {}, {FileAccessCheck::SameDirectory, FileAccessCheck::Subdirectories}, false},
            {TestFile::WrongSuffix, {}, {FileAccessCheck::SameDirectory, FileAccessCheck::Subdirectories}, false},
            //
            {TestFile::ParentDirectory, {FileAccessCheck::AnyDirectory}, {}, true}, // 16
            {TestFile::SameDirectory, {FileAccessCheck::AnyDirectory}, {}, true},
            {TestFile::Subdirectory, {FileAccessCheck::AnyDirectory}, {}, true},
            {TestFile::WrongSuffix, {FileAccessCheck::AnyDirectory}, {}, true},
            //
            {TestFile::ParentDirectory,
                {FileAccessCheck::AnyDirectory},
                {FileAccessCheck::SameDirectory, FileAccessCheck::Subdirectories},
                true}, // 20
            {TestFile::SameDirectory,
                {FileAccessCheck::AnyDirectory},
                {FileAccessCheck::SameDirectory, FileAccessCheck::Subdirectories},
                true},
            {TestFile::Subdirectory,
                {FileAccessCheck::AnyDirectory},
                {FileAccessCheck::SameDirectory, FileAccessCheck::Subdirectories},
                true},
            {TestFile::WrongSuffix,
                {FileAccessCheck::AnyDirectory},
                {FileAccessCheck::SameDirectory, FileAccessCheck::Subdirectories},
                true},
            //
            {TestFile::ParentDirectory, {FileAccessCheck::RequireSuffix}, {}, false}, // 24
            {TestFile::SameDirectory, {FileAccessCheck::RequireSuffix}, {}, true},
            {TestFile::Subdirectory, {FileAccessCheck::RequireSuffix}, {}, true},
            {TestFile::WrongSuffix, {FileAccessCheck::RequireSuffix}, {}, false},
        };
        int index = 0;
        for (const auto &data : testData) {
            runWithContext(
                SOURCE_LOCATION(),
                [&] { verifyAccess(data); },
                [&]() -> std::string { return std::format("Failed for test case {}", index); });
            cleanUpTestFileDirectory();
            index += 1;
        }
    }

    void testRequireFileSources() {
        // Prepare the test environment.
        el::text::String document = "[main]\nvalue: 123\n"_el;
        // Create a custom access check.
        auto accessCheck = FileAccessCheck::create();
        accessCheck->enable(FileAccessCheck::OnlyFileSources);
        // Set up the parser
        Parser parser;
        parser.setAccessCheck(accessCheck);
        auto source = Source::fromString(document);
        try {
            doc = parser.parseOrThrow(source);
            REQUIRE(false);
        } catch (const ConfError &error) {
            REQUIRE_EQUAL(error.category(), ConfErrorCategory::Access);
        }
    }
};
