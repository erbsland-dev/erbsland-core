// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../ConfTestHelper.hpp"

#include <erbsland/conf/FileSourceResolver.hpp>
#include <erbsland/conf/StdFormat.hpp>
#include <erbsland/err/ParameterError.hpp>
#include <erbsland/path/Path.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/text/StringEditor.hpp>
#include <erbsland/text/StringList.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace el::conf;
using namespace el::text::literals;

TESTED_TARGETS(FileSourceResolver)
class FileSourceResolverTest final : public UNITTEST_SUBCLASS(ConfTestHelper) {
public:
    void testInvalidFeaturesAreParameterErrors() {
        auto resolver = FileSourceResolver::create();
        const auto invalidFeature = static_cast<FileSourceResolver::Feature>(FileSourceResolver::_featureCount);
        REQUIRE_THROWS_AS(el::err::ParameterError, resolver->enable(invalidFeature));
        REQUIRE_THROWS_AS(el::err::ParameterError, resolver->disable(invalidFeature));
        REQUIRE_THROWS_AS(el::err::ParameterError, resolver->isEnabled(invalidFeature));
    }

    [[nodiscard]] static auto concatenate(std::initializer_list<el::text::String> parts) -> el::text::String {
        auto result = el::text::StringEditor{};
        for (const auto &part : parts) {
            result.append(part);
        }
        return result;
    }

    void tearDown() override { cleanUpTestFileDirectory(); }

    using FileList = std::vector<std::string>;
    using ExpectedSourceList = el::text::StringList;

    SourceListPtr sources;
    SourceIdentifierPtr documentSourceIdentifier;
    std::vector<std::pair<el::text::String, el::text::String>> actualSourceList;

    auto createTestFile(const std::string &relativePath) -> std::filesystem::path {
        const auto basePath = useTestFileDirectory();
        auto filePath = basePath / relativePath;
        std::filesystem::create_directories(filePath.parent_path());
        std::ofstream stream(filePath);
        stream << "# Erbsland Configuration Language - Test File\n";
        stream << "[main]\n";
        stream << "value = 123\n";
        stream.close();
        return filePath;
    }

    auto createSourceIdentifier(const std::filesystem::path &path) {
        auto canonicalPath = canonical(path);
        documentSourceIdentifier = SourceIdentifier::createForFile(el::path::Path{canonicalPath}.toString());
    }

    void setupFileList(const FileList &fileList) {
        // Ensure test isolation: wildcard resolving scans the filesystem, so leftovers from
        // previous tests would change the result set.
        cleanUpTestFileDirectory();
        documentSourceIdentifier = {};
        for (const auto &path : fileList) {
            auto filePath = createTestFile(path);
            if (documentSourceIdentifier == nullptr) {
                createSourceIdentifier(filePath);
            }
        }
        REQUIRE(documentSourceIdentifier);
    }

    void expectSuccess(const el::text::String &includeText, const ExpectedSourceList &expected) {

        auto resolver = FileSourceResolver::create();
        REQUIRE(resolver);
        const SourceResolverContext context{.includeText = includeText, .sourceIdentifier = documentSourceIdentifier};
        const auto basePath = useTestFileDirectory();
        actualSourceList = {};
        runWithContext(
            SOURCE_LOCATION(),
            [&]() {
                REQUIRE_NOTHROW(sources = resolver->resolve(context));
                for (size_t i = 0; i < sources->size(); ++i) {
                    auto sourcePathStr = sources->at(i)->path();
                    auto sourcePath = el::path::Path{sourcePathStr}.toStdPath();
                    auto relSourcePath = relative(sourcePath, basePath);
                    actualSourceList.emplace_back(relSourcePath.generic_string(), sourcePathStr);
                }
                REQUIRE_EQUAL(sources->size(), expected.count().toSizeT());
                for (size_t i = 0; i < sources->size(); ++i) {
                    auto sourcePathStr = sources->at(i)->path();
                    REQUIRE_FALSE(sourcePathStr.isEmpty());
                    auto sourcePath = el::path::Path{sourcePathStr}.toStdPath();
                    REQUIRE(sourcePath.is_absolute());
                    auto relSourcePath = relative(sourcePath, basePath);
                    REQUIRE_FALSE(relSourcePath.empty());
                    REQUIRE_EQUAL(
                        relSourcePath.generic_string(),
                        el::text::StringConverter{expected.getRefOrThrow(el::unit::ItemIndex::fromSizeT(i))}
                            .toStdString());
                }
            },
            [&]() -> std::string {
                std::string result;
                result += std::format("Include: {}\n", el::text::StringConverter{context.includeText}.toStdString());
                result += "Expected Sources:\n";
                int index = 0;
                for (const auto &source : expected) {
                    result += std::format("  {}: {}\n", index++, el::text::StringConverter{source}.toStdString());
                }
                result += "Actual Sources:\n";
                if (actualSourceList.empty()) {
                    result += "  <EMPTY>\n";
                } else {
                    index = 0;
                    for (const auto &[relPath, absPath] : actualSourceList) {
                        auto path = relPath.isEmpty() ? absPath : relPath;
                        result += std::format("  {}: {}\n", index++, el::text::StringConverter{path}.toStdString());
                    }
                }
                return result;
            });
    }

    void expectSuccess(const el::text::String &includeText, const std::initializer_list<el::text::String> expected) {
        expectSuccess(includeText, ExpectedSourceList{expected});
    }

    void expectSuccessVariants(const el::text::String &includeText, const ExpectedSourceList &expected) {
        el::text::StringList variants;
        variants.append(includeText);
        variants.append(concatenate({"file:"_el, includeText}));
        const auto testDirectory = el::path::Path{useTestFileDirectory()}.toString();
        variants.append(concatenate({testDirectory, "/config/"_el, includeText}));
        variants.append(concatenate({"file:"_el, testDirectory, "/config/"_el, includeText}));
        for (const auto &variant : variants) {
            WITH_CONTEXT(expectSuccess(variant, expected));
        }
    }

    void expectSuccessVariants(
        const el::text::String &includeText, const std::initializer_list<el::text::String> expected) {
        expectSuccessVariants(includeText, ExpectedSourceList{expected});
    }

    void expectSuccessAbs(const el::text::String &includeText, const ExpectedSourceList &expected) {

        el::text::StringEditor absIncludeText;
        if (includeText.startsWith("file:"_el)) {
            absIncludeText.append("file:"_el);
        }
        absIncludeText.append(el::path::Path{useTestFileDirectory()}.toString());
        absIncludeText.append("/"_el);
        if (includeText.startsWith("file:"_el)) {
            absIncludeText.append(includeText.splitAt(el::unit::ByteIndex{5}).second);
        } else {
            absIncludeText.append(includeText);
        }
        expectSuccess(el::text::String{absIncludeText}, expected);
    }

    void expectSuccessAbs(const el::text::String &includeText, const std::initializer_list<el::text::String> expected) {
        expectSuccessAbs(includeText, ExpectedSourceList{expected});
    }

    void expectFailure(
        const el::text::String &includeText, ConfErrorCategory expectedErrorCategory = ConfErrorCategory::Syntax) {
        auto resolver = FileSourceResolver::create();
        REQUIRE(resolver);
        const SourceResolverContext context{.includeText = includeText, .sourceIdentifier = documentSourceIdentifier};
        bool caughtError = false;
        runWithContext(
            SOURCE_LOCATION(),
            [&]() {
                try {
                    sources = resolver->resolve(context);
                    REQUIRE(false);
                } catch (const ConfError &error) {
                    caughtError = true;
                    REQUIRE_EQUAL(error.category(), expectedErrorCategory);
                }
            },
            [&]() -> std::string {
                std::string result;
                if (!caughtError) {
                    result += "Expected error not thrown.\n";
                }
                result += std::format("Include: {}\n", el::text::StringConverter{context.includeText}.toStdString());
                result += "Sources:\n";
                if (sources == nullptr) {
                    result += "  <NULL>\n";
                } else {
                    int index = 0;
                    for (const auto &source : *sources) {
                        result +=
                            std::format("  {}: {}\n", index++, el::text::StringConverter{source->path()}.toStdString());
                    }
                }
                return result;
            });
    }

    void testIncorrectInput() {
        auto resolver = FileSourceResolver::create();
        REQUIRE(resolver);
        REQUIRE_THROWS(resolver->resolve({}));
        REQUIRE_THROWS(resolver->resolve({.includeText = "test.elcl"_el, .sourceIdentifier = {}}));
        REQUIRE_THROWS(
            resolver->resolve({.includeText = "test.elcl"_el, .sourceIdentifier = SourceIdentifier::createForText()}));
        REQUIRE_THROWS(resolver->resolve(
            {.includeText = "test.elcl"_el, .sourceIdentifier = SourceIdentifier::createForFile("relative.elcl"_el)}));
        REQUIRE_THROWS(resolver->resolve(
            {.includeText = "test.elcl"_el, .sourceIdentifier = SourceIdentifier::createForFile("/"_el)}));
        REQUIRE_THROWS(resolver->resolve(
            {.includeText = "test.elcl"_el,
                .sourceIdentifier = SourceIdentifier::createForFile("/invalid/path/a/b/c/d/e/relative.elcl"_el)}));
        REQUIRE_THROWS(resolver->resolve(
            {.includeText = "test.elcl"_el, .sourceIdentifier = SourceIdentifier::createForFile("/document.elcl"_el)}));
        const auto fileList = FileList{
            "config/document.elcl",
        };
        auto doubleFilePath = useTestFileDirectory() / "config/document.elcl/document.elcl";
        REQUIRE_THROWS(resolver->resolve(
            {.includeText = "test.elcl"_el,
                .sourceIdentifier = SourceIdentifier::createForFile(el::path::Path{doubleFilePath}.toString())}));
        setupFileList(fileList);
    }

    void testOneAbsolutePath() {
        // before starting with automated tests, do a manual test to make sure the test methods work as expected.
        auto documentPath = createTestFile("config/document.elcl");
        REQUIRE(!documentPath.empty());
        REQUIRE(documentPath.is_absolute());
        REQUIRE(is_regular_file(documentPath));
        createSourceIdentifier(documentPath);
        REQUIRE(documentSourceIdentifier);
        REQUIRE_EQUAL(documentSourceIdentifier->name(), "file"_el);
        const auto includedFile = createTestFile("config/IncludedFile.elcl");
        REQUIRE(is_regular_file(includedFile));
        const auto resolver = FileSourceResolver::create();
        REQUIRE(resolver);
        const auto basePath = el::path::Path{useTestFileDirectory()}.toString();
        REQUIRE(!basePath.isEmpty());
        const SourceResolverContext context{
            .includeText = concatenate({basePath, "/config/IncludedFile.elcl"_el}),
            .sourceIdentifier = documentSourceIdentifier,
        };
        SourceListPtr sourceList;
        REQUIRE_NOTHROW(sourceList = resolver->resolve(context));
        REQUIRE(sourceList);
        REQUIRE_EQUAL(sourceList->size(), 1);
        auto source = sourceList->at(0);
        REQUIRE(source);
        auto pathFromSourceList = source->path();
        auto actualPathOfInclude = el::path::Path{canonical(includedFile)}.toString();
        REQUIRE_EQUAL(actualPathOfInclude, pathFromSourceList);
    }

    void testMaximumWildcards() {
        // manually test what happens when using the pattern "**/*"
        auto documentPath = createTestFile("config/document.elcl");
        REQUIRE(!documentPath.empty());
        REQUIRE(documentPath.is_absolute());
        REQUIRE(is_regular_file(documentPath));
        createSourceIdentifier(documentPath);
        REQUIRE(documentSourceIdentifier);
        REQUIRE_EQUAL(documentSourceIdentifier->name(), "file"_el);
        createTestFile("config/file1.elcl");
        createTestFile("config/file2.elcl");
        createTestFile("config/file3.elcl");
        const auto resolver = FileSourceResolver::create();
        REQUIRE(resolver);
        const SourceResolverContext context{
            .includeText = "**/*"_el,
            .sourceIdentifier = documentSourceIdentifier,
        };
        SourceListPtr sourceList;
        REQUIRE_NOTHROW(sourceList = resolver->resolve(context));
        REQUIRE(sourceList);
        REQUIRE_EQUAL(sourceList->size(), 4);
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testPlainPaths() {
        const auto fileList = FileList{
            "config/MainDocument.elcl",
            "config/SameDir.elcl",
            "config/SubDir/SubDirDocument.elcl",
            "ParentDocument.elcl",
            "other/OtherDocument.elcl"};
        setupFileList(fileList);
        // plain and simple
        WITH_CONTEXT(expectSuccessVariants("SameDir.elcl"_el, {"config/SameDir.elcl"_el}));
        WITH_CONTEXT(expectSuccessVariants("SubDir/SubDirDocument.elcl"_el, {"config/SubDir/SubDirDocument.elcl"_el}));
        WITH_CONTEXT(expectSuccessVariants("../ParentDocument.elcl"_el, {"ParentDocument.elcl"_el}));
        WITH_CONTEXT(expectSuccessVariants("../other/OtherDocument.elcl"_el, {"other/OtherDocument.elcl"_el}));
        WITH_CONTEXT(expectSuccessVariants("..//other//////OtherDocument.elcl"_el, {"other/OtherDocument.elcl"_el}));
        WITH_CONTEXT(
            expectSuccessVariants("../////////other///OtherDocument.elcl"_el, {"other/OtherDocument.elcl"_el}));
        // normalization required
        WITH_CONTEXT(expectSuccessVariants("./SameDir.elcl"_el, {"config/SameDir.elcl"_el}));
        WITH_CONTEXT(expectSuccessVariants(".//SameDir.elcl"_el, {"config/SameDir.elcl"_el}));
        WITH_CONTEXT(expectSuccessVariants(".\\SameDir.elcl"_el, {"config/SameDir.elcl"_el}));
        WITH_CONTEXT(expectSuccessVariants("./././SameDir.elcl"_el, {"config/SameDir.elcl"_el}));
        WITH_CONTEXT(expectSuccessVariants(".//////./////.///SameDir.elcl"_el, {"config/SameDir.elcl"_el}));
        WITH_CONTEXT(expectSuccessVariants("SubDir/../SameDir.elcl"_el, {"config/SameDir.elcl"_el}));
        WITH_CONTEXT(expectSuccessVariants("SubDir\\..\\.\\SameDir.elcl"_el, {"config/SameDir.elcl"_el}));
        WITH_CONTEXT(expectSuccessVariants(
            "./SubDir/../../config/SubDir/SubDirDocument.elcl"_el, {"config/SubDir/SubDirDocument.elcl"_el}));
        WITH_CONTEXT(expectSuccessVariants("../other/../ParentDocument.elcl"_el, {"ParentDocument.elcl"_el}));
        WITH_CONTEXT(expectSuccessVariants(".\\..\\other\\OtherDocument.elcl"_el, {"other/OtherDocument.elcl"_el}));
    }

    void testPlainPathsLight() {
        setupFileList({"config/MainDocument.elcl", "config/SameDir.elcl", "other/OtherDocument.elcl"});
        WITH_CONTEXT(expectSuccess("SameDir.elcl"_el, {"config/SameDir.elcl"_el}));
        WITH_CONTEXT(expectSuccess("../other/OtherDocument.elcl"_el, {"other/OtherDocument.elcl"_el}));
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testFilenameWildcards() {
        const auto fileList = FileList{
            "config/MainDocument.elcl",
            "config/sub/a/doc001.elcl",
            "config/sub/a/doc002.elcl",
            "config/sub/b/doc003.elcl",
            "config/sub/b/doc004.elcl",
            "config/sub/doc005.elcl",
            "config/sub/doc006.elcl",
            "config/sub/conf007.elcl",
            "config/sub/conf008.txt",
            "config/doc009.elcl",
            "config/doc010.elcl",
            "config/doc011.elcl",
            "config/doc012.elcl",
            "doc013.elcl",
            "doc014.elcl",
            "config.txt",
        };
        setupFileList(fileList);
        WITH_CONTEXT(expectSuccess(
            "*"_el,
            {
                "config/MainDocument.elcl"_el,
                "config/doc009.elcl"_el,
                "config/doc010.elcl"_el,
                "config/doc011.elcl"_el,
                "config/doc012.elcl"_el,
            }));
        WITH_CONTEXT(expectSuccess(
            "*.elcl"_el,
            {
                "config/MainDocument.elcl"_el,
                "config/doc009.elcl"_el,
                "config/doc010.elcl"_el,
                "config/doc011.elcl"_el,
                "config/doc012.elcl"_el,
            }));
        WITH_CONTEXT(expectSuccess(
            "doc*"_el,
            {
                "config/doc009.elcl"_el,
                "config/doc010.elcl"_el,
                "config/doc011.elcl"_el,
                "config/doc012.elcl"_el,
            }));
        WITH_CONTEXT(expectSuccess(
            "doc*.elcl"_el,
            {
                "config/doc009.elcl"_el,
                "config/doc010.elcl"_el,
                "config/doc011.elcl"_el,
                "config/doc012.elcl"_el,
            }));
        WITH_CONTEXT(expectSuccess(
            "../*"_el,
            {
                "config.txt"_el,
                "doc013.elcl"_el,
                "doc014.elcl"_el,
            }));
        WITH_CONTEXT(expectSuccess(
            "../*4.elcl"_el,
            {
                "doc014.elcl"_el,
            }));
        WITH_CONTEXT(expectSuccess(
            "../doc*"_el,
            {
                "doc013.elcl"_el,
                "doc014.elcl"_el,
            }));
        WITH_CONTEXT(expectSuccess(
            "../doc*3.elcl"_el,
            {
                "doc013.elcl"_el,
            }));
        WITH_CONTEXT(expectSuccess(
            "sub/*"_el,
            {
                "config/sub/conf007.elcl"_el,
                "config/sub/conf008.txt"_el,
                "config/sub/doc005.elcl"_el,
                "config/sub/doc006.elcl"_el,
            }));
        WITH_CONTEXT(expectSuccess("sub/*4.elcl"_el, {}));
        WITH_CONTEXT(expectSuccess(
            "sub/*.elcl"_el,
            {
                "config/sub/conf007.elcl"_el,
                "config/sub/doc005.elcl"_el,
                "config/sub/doc006.elcl"_el,
            }));
        WITH_CONTEXT(expectSuccess(
            "sub/d*"_el,
            {
                "config/sub/doc005.elcl"_el,
                "config/sub/doc006.elcl"_el,
            }));
        WITH_CONTEXT(expectSuccess(
            "sub/doc00*l"_el,
            {
                "config/sub/doc005.elcl"_el,
                "config/sub/doc006.elcl"_el,
            }));
        // same with absolute paths
        WITH_CONTEXT(expectSuccessAbs(
            "config/*"_el,
            {
                "config/MainDocument.elcl"_el,
                "config/doc009.elcl"_el,
                "config/doc010.elcl"_el,
                "config/doc011.elcl"_el,
                "config/doc012.elcl"_el,
            }));
        WITH_CONTEXT(expectSuccessAbs(
            "config/*.elcl"_el,
            {
                "config/MainDocument.elcl"_el,
                "config/doc009.elcl"_el,
                "config/doc010.elcl"_el,
                "config/doc011.elcl"_el,
                "config/doc012.elcl"_el,
            }));
        WITH_CONTEXT(expectSuccessAbs(
            "config/doc*"_el,
            {
                "config/doc009.elcl"_el,
                "config/doc010.elcl"_el,
                "config/doc011.elcl"_el,
                "config/doc012.elcl"_el,
            }));
        WITH_CONTEXT(expectSuccessAbs(
            "config/doc*.elcl"_el,
            {
                "config/doc009.elcl"_el,
                "config/doc010.elcl"_el,
                "config/doc011.elcl"_el,
                "config/doc012.elcl"_el,
            }));
        WITH_CONTEXT(expectSuccessAbs(
            "config/../*"_el,
            {
                "config.txt"_el,
                "doc013.elcl"_el,
                "doc014.elcl"_el,
            }));
        WITH_CONTEXT(expectSuccessAbs(
            "config/../*4.elcl"_el,
            {
                "doc014.elcl"_el,
            }));
        WITH_CONTEXT(expectSuccessAbs(
            "config/../doc*"_el,
            {
                "doc013.elcl"_el,
                "doc014.elcl"_el,
            }));
        WITH_CONTEXT(expectSuccessAbs(
            "config/../doc*3.elcl"_el,
            {
                "doc013.elcl"_el,
            }));
        WITH_CONTEXT(expectSuccessAbs(
            "config/sub/*"_el,
            {
                "config/sub/conf007.elcl"_el,
                "config/sub/conf008.txt"_el,
                "config/sub/doc005.elcl"_el,
                "config/sub/doc006.elcl"_el,
            }));
        WITH_CONTEXT(expectSuccessAbs("config/sub/*4.elcl"_el, {}));
        WITH_CONTEXT(expectSuccessAbs(
            "config/sub/*.elcl"_el,
            {
                "config/sub/conf007.elcl"_el,
                "config/sub/doc005.elcl"_el,
                "config/sub/doc006.elcl"_el,
            }));
        WITH_CONTEXT(expectSuccessAbs(
            "config/sub/d*"_el,
            {
                "config/sub/doc005.elcl"_el,
                "config/sub/doc006.elcl"_el,
            }));
        WITH_CONTEXT(expectSuccessAbs(
            "config/sub/doc00*l"_el,
            {
                "config/sub/doc005.elcl"_el,
                "config/sub/doc006.elcl"_el,
            }));
    }

    void testFilenameWildcardsLight() {
        setupFileList({"config/MainDocument.elcl", "config/doc001.elcl", "config/readme.txt"});
        WITH_CONTEXT(expectSuccess("*.elcl"_el, {"config/MainDocument.elcl"_el, "config/doc001.elcl"_el}));
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testDirectoryWildcards() {
        const auto fileList = FileList{
            "config/doc009.elcl",
            "config/doc010.elcl",
            "config/doc011.elcl",
            "config/doc012.elcl",
            "config/main.elcl",
            "config/sub/a/doc001.elcl",
            "config/sub/a/doc002.elcl",
            "config/sub/b/doc003.elcl",
            "config/sub/b/doc004.elcl",
            "config/sub/conf007.elcl",
            "config/sub/conf008.txt",
            "config/sub/doc005.elcl",
            "config/sub/doc006.elcl",
            "config.txt",
            "doc013.elcl",
            "doc014.elcl",
        };
        setupFileList(fileList);
        WITH_CONTEXT(expectSuccessVariants(
            "**/*"_el,
            {
                "config/doc009.elcl"_el,
                "config/doc010.elcl"_el,
                "config/doc011.elcl"_el,
                "config/doc012.elcl"_el,
                "config/main.elcl"_el,
                "config/sub/conf007.elcl"_el,
                "config/sub/conf008.txt"_el,
                "config/sub/doc005.elcl"_el,
                "config/sub/doc006.elcl"_el,
                "config/sub/a/doc001.elcl"_el,
                "config/sub/a/doc002.elcl"_el,
                "config/sub/b/doc003.elcl"_el,
                "config/sub/b/doc004.elcl"_el,
            }));
        WITH_CONTEXT(expectSuccessVariants(
            "../**/*"_el,
            {
                "config.txt"_el,
                "doc013.elcl"_el,
                "doc014.elcl"_el,
                "config/doc009.elcl"_el,
                "config/doc010.elcl"_el,
                "config/doc011.elcl"_el,
                "config/doc012.elcl"_el,
                "config/main.elcl"_el,
                "config/sub/conf007.elcl"_el,
                "config/sub/conf008.txt"_el,
                "config/sub/doc005.elcl"_el,
                "config/sub/doc006.elcl"_el,
                "config/sub/a/doc001.elcl"_el,
                "config/sub/a/doc002.elcl"_el,
                "config/sub/b/doc003.elcl"_el,
                "config/sub/b/doc004.elcl"_el,
            }));
        WITH_CONTEXT(expectSuccessVariants(
            "sub/**/*"_el,
            {
                "config/sub/conf007.elcl"_el,
                "config/sub/conf008.txt"_el,
                "config/sub/doc005.elcl"_el,
                "config/sub/doc006.elcl"_el,
                "config/sub/a/doc001.elcl"_el,
                "config/sub/a/doc002.elcl"_el,
                "config/sub/b/doc003.elcl"_el,
                "config/sub/b/doc004.elcl"_el,
            }));
        WITH_CONTEXT(expectSuccessVariants(
            "**/doc006.elcl"_el,
            {
                "config/sub/doc006.elcl"_el,
            }));
        WITH_CONTEXT(expectSuccessVariants("**/doc999.elcl"_el, {}));
        WITH_CONTEXT(expectSuccessVariants(
            "../**/doc004.elcl"_el,
            {
                "config/sub/b/doc004.elcl"_el,
            }));
        WITH_CONTEXT(expectSuccessVariants(
            "sub/**/conf008.txt"_el,
            {
                "config/sub/conf008.txt"_el,
            }));
        WITH_CONTEXT(expectSuccessVariants(
            "**/*.txt"_el,
            {
                "config/sub/conf008.txt"_el,
            }));
        WITH_CONTEXT(expectSuccessVariants(
            "../**/doc*"_el,
            {
                "doc013.elcl"_el,
                "doc014.elcl"_el,
                "config/doc009.elcl"_el,
                "config/doc010.elcl"_el,
                "config/doc011.elcl"_el,
                "config/doc012.elcl"_el,
                "config/sub/doc005.elcl"_el,
                "config/sub/doc006.elcl"_el,
                "config/sub/a/doc001.elcl"_el,
                "config/sub/a/doc002.elcl"_el,
                "config/sub/b/doc003.elcl"_el,
                "config/sub/b/doc004.elcl"_el,
            }));
        WITH_CONTEXT(expectSuccessVariants(
            "sub/**/*.elcl"_el,
            {
                "config/sub/conf007.elcl"_el,
                "config/sub/doc005.elcl"_el,
                "config/sub/doc006.elcl"_el,
                "config/sub/a/doc001.elcl"_el,
                "config/sub/a/doc002.elcl"_el,
                "config/sub/b/doc003.elcl"_el,
                "config/sub/b/doc004.elcl"_el,
            }));
    }

    void testDirectoryWildcardsLight() {
        setupFileList({"config/main.elcl", "config/sub/document.elcl", "config/sub/readme.txt"});
        WITH_CONTEXT(expectSuccess("**/*.elcl"_el, {"config/main.elcl"_el, "config/sub/document.elcl"_el}));
    }

    void testErrors() {
        const auto fileList = FileList{
            "config/MainDocument.elcl",
            "config/SameDir.elcl",
            "config/SubDir/SubDirDocument.elcl",
            "config/SubDir/A/a.elcl",
            "config/SubDir/B/b.elcl",
            "ParentDocument.elcl",
            "other/OtherDocument.elcl"};
        setupFileList(fileList);
        // Test general errors.
        WITH_CONTEXT(expectFailure(""_el));
        WITH_CONTEXT(expectFailure("file:"_el));
        WITH_CONTEXT(expectFailure("."_el));
        WITH_CONTEXT(expectFailure(".."_el));
        WITH_CONTEXT(expectFailure("../../../../../../../../../config/SameDir.elcl"_el));
        WITH_CONTEXT(expectFailure("config/SameDir.elcl"_el));
        WITH_CONTEXT(expectFailure("SameDir.elcl/"_el));
        WITH_CONTEXT(expectFailure("../SameDir.elcl/"_el));
        WITH_CONTEXT(expectFailure("SubDir/"_el));
        WITH_CONTEXT(expectFailure("../SubDir/"_el));
        // Test invalid wildcards
        WITH_CONTEXT(expectFailure("Sub*Dir/SubDirDocument.elcl"_el));
        WITH_CONTEXT(expectFailure("Sub**Dir/SubDirDocument.elcl"_el));
        WITH_CONTEXT(expectFailure("Sub***Dir/SubDirDocument.elcl"_el));
        WITH_CONTEXT(expectFailure("**/**/SubDirDocument.elcl"_el));
        WITH_CONTEXT(expectFailure("**/**/SubDirDocument.elcl"_el));
        WITH_CONTEXT(expectFailure("SubDir/**/**/SubDirDocument.elcl"_el));
        WITH_CONTEXT(expectFailure("*/**/SubDirDocument.elcl"_el));
        WITH_CONTEXT(expectFailure("Sub**/SubDirDocument.elcl"_el));
        WITH_CONTEXT(expectFailure("**Dir/SubDirDocument.elcl"_el));
        WITH_CONTEXT(expectFailure("SubDir/S*D*Document.elcl"_el));
        WITH_CONTEXT(expectFailure("SubDir/S**Document.elcl"_el));
        WITH_CONTEXT(expectFailure("SubDir/S***Document.elcl"_el));
        // Test invalid UNC paths
        WITH_CONTEXT(expectFailure("//"_el));
        WITH_CONTEXT(expectFailure("///"_el));
        WITH_CONTEXT(expectFailure("///config.elcl"_el));
        WITH_CONTEXT(expectFailure("//a"_el));
        WITH_CONTEXT(expectFailure("//abc"_el));
        WITH_CONTEXT(expectFailure("//abc/"_el));
        WITH_CONTEXT(expectFailure("//local*host/config.elcl"_el));
        WITH_CONTEXT(expectFailure("//local?host/config.elcl"_el));
        WITH_CONTEXT(expectFailure("//local|host/config.elcl"_el));
        WITH_CONTEXT(expectFailure("//local\"host/config.elcl"_el));
        WITH_CONTEXT(expectFailure("//local<host/config.elcl"_el));
        WITH_CONTEXT(expectFailure("//local😀host/config.elcl"_el));
        WITH_CONTEXT(expectFailure("\\\\"_el));
        WITH_CONTEXT(expectFailure("\\\\\\"_el));
        WITH_CONTEXT(expectFailure("\\\\\\config.elcl"_el));
        WITH_CONTEXT(expectFailure("\\\\a"_el));
        WITH_CONTEXT(expectFailure("\\\\abc"_el));
        WITH_CONTEXT(expectFailure("\\\\abc\\"_el));
        WITH_CONTEXT(expectFailure("\\\\local*host\\config.elcl"_el));
        WITH_CONTEXT(expectFailure("\\\\local?host\\config.elcl"_el));
        WITH_CONTEXT(expectFailure("\\\\local|host\\config.elcl"_el));
        WITH_CONTEXT(expectFailure("\\\\local\"host\\config.elcl"_el));
        WITH_CONTEXT(expectFailure("\\\\local<host\\config.elcl"_el));
        WITH_CONTEXT(expectFailure("\\\\local😀host\\config.elcl"_el));
    }
};
