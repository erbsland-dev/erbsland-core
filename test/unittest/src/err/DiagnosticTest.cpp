// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/err/DiagnosticHelper.hpp>
#include <erbsland/err/Exception.hpp>
#include <erbsland/err/ParseError.hpp>
#include <erbsland/i18n/DisplayTextMap.hpp>
#include <erbsland/path/PathError.hpp>
#include <erbsland/path/PathErrorContext.hpp>
#include <erbsland/stream/StreamError.hpp>
#include <erbsland/stream/StreamErrorContext.hpp>
#include <erbsland/system/PlatformError.hpp>
#include <erbsland/system/PosixErrorContext.hpp>

#if defined(_WIN32)
#include <erbsland/system/WindowsErrorContext.hpp>
#endif
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/text/TextDocument.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <cerrno>
#include <exception>
#include <stdexcept>
#include <string>

using namespace el::text::literals;

TESTED_TARGETS(
    Diagnostic Exception PathError PathErrorContext StreamError StreamErrorContext PlatformError PosixErrorContext
        PlatformErrorCategory)
class DiagnosticTest final : public el::UnitTest {
public:
    void testDefaultExceptionDiagnostic() {
        const auto error = el::err::Exception{"Broken value"_el};
        const auto diagnostic = error.diagnostic();

        REQUIRE(diagnostic != nullptr);
        REQUIRE_EQUAL(toStdString(diagnostic->toString()), std::string{"Broken value"});
        REQUIRE_EQUAL(toStdString(diagnostic->toTextDocument().toString()), std::string{"Error: Broken value"});
        REQUIRE(diagnostic->sourceName().isEmpty());
        REQUIRE(diagnostic->sourcePath().isEmpty());
        REQUIRE(diagnostic->location().line.isNoIndex());
    }

    void testParseErrorDiagnosticLocation() {
        const auto error = el::err::ParseError{"Invalid integer"_el, el::unit::CpIndex{4U}};
        const auto diagnostic = error.diagnostic();

        REQUIRE(diagnostic != nullptr);
        REQUIRE_EQUAL(diagnostic->location().position, el::unit::CpIndex{4U});
        REQUIRE_EQUAL(toStdString(diagnostic->toString()), std::string{"Invalid integer at code point 4"});
    }

    void testPathDiagnosticPathMetadata() {
        const auto error = el::path::PathError{
            el::path::PathErrorContext{"File could not be read"_el, "The file is unavailable."_el}.setSourcePath(
                "/tmp/missing.txt"_el)};
        const auto diagnostic = error.diagnostic();

        REQUIRE(diagnostic != nullptr);
        REQUIRE_EQUAL(toStdString(diagnostic->sourcePath()), std::string{"/tmp/missing.txt"});
        const auto document = diagnostic->toTextDocument();
        const auto text = toStdString(document.toString());
        WITH_CONTEXT(requireContains(text, "File could not be read"));
        WITH_CONTEXT(requireContains(text, "path"));
        WITH_CONTEXT(requireContains(text, "/tmp/missing.txt"));
        WITH_CONTEXT(requireContains(text, "Paths:"));
        REQUIRE(document.root()->contains(el::text::TextNodeType::Separator));
    }

    void testPathDiagnosticEmbedsPlatformContext() {
        auto context = std::make_shared<const el::system::PosixErrorContext>(2, "No such file"_el);
        auto error =
            el::path::PathError{el::path::PathErrorContext{"File could not be read"_el, "The file is unavailable."_el}
                    .setSourcePath("/tmp/missing.txt"_el)
                    .setPlatformContext(context)};

        REQUIRE_FALSE(error.hasCause());
        REQUIRE_EQUAL(toStdString(error.toString()), std::string{"File could not be read"});
        REQUIRE_EQUAL(error.platformContext(), context);
        REQUIRE_EQUAL(error.help(), "Check that the path is correct and that every parent directory exists."_el);

        const auto text = toStdString(el::err::DiagnosticHelper{error}.toDocument().toString());

        WITH_CONTEXT(requireContains(text, "File could not be read"));
        WITH_CONTEXT(requireContains(text, "The file is unavailable."));
        WITH_CONTEXT(requireContains(text, "path"));
        WITH_CONTEXT(requireContains(text, "/tmp/missing.txt"));
        REQUIRE(text.find("Caused By") == std::string::npos);
        WITH_CONTEXT(requireContains(text, "Paths:"));
        WITH_CONTEXT(requireContains(text, "Platform Error:"));
        WITH_CONTEXT(requireContains(text, "errno"));
        WITH_CONTEXT(requireContains(text, "2"));
        WITH_CONTEXT(requireContains(text, "message"));
        WITH_CONTEXT(requireContains(text, "No such file"));
        WITH_CONTEXT(requireContains(text, "Paths:\n  path:"));
        WITH_CONTEXT(requireContains(text, "Platform Error:\n  errno:"));
    }

    void testPathContextSourceTargetAndExplicitHelp() {
        auto context = el::path::PathErrorContext{"File or directory could not be copied"_el, "Copy failed."_el};
        context.setSourcePath("/from/a"_el)
            .setTargetPath("/to/b"_el)
            .setHelp("Choose another destination."_el)
            .setPlatformContext(std::make_shared<const el::system::PosixErrorContext>(17, "Exists"_el));
        const auto error = el::path::PathError{context};
        const auto text = toStdString(error.diagnostic()->toTextDocument().toString());

        REQUIRE_EQUAL(error.sourcePath(), "/from/a"_el);
        REQUIRE_EQUAL(error.targetPath(), "/to/b"_el);
        REQUIRE_EQUAL(error.help(), "Choose another destination."_el);
        WITH_CONTEXT(requireContains(text, "source path"));
        WITH_CONTEXT(requireContains(text, "target path"));
        WITH_CONTEXT(requireContains(text, "Choose another destination."));
    }

    void testPathContextNoPathAndTargetOnlyLabels() {
        const auto noPath = el::path::PathError{el::path::PathErrorContext{"Path operation failed"_el}};
        const auto noPathText = toStdString(noPath.diagnostic()->toTextDocument().toString());
        REQUIRE(noPathText.find("target path") == std::string::npos);
        REQUIRE(noPathText.find("source path") == std::string::npos);

        const auto targetOnly = el::path::PathError{
            el::path::PathErrorContext{"Output could not be created"_el}.setTargetPath("/tmp/output"_el)};
        const auto targetText = toStdString(targetOnly.diagnostic()->toTextDocument().toString());
        WITH_CONTEXT(requireContains(targetText, "target path"));
        WITH_CONTEXT(requireContains(targetText, "/tmp/output"));
        REQUIRE(targetText.find("source path") == std::string::npos);
    }

    void testPathContextCanKeepIndependentCause() {
        auto cause = std::make_exception_ptr(el::err::Exception{"Configuration lookup failed"_el});
        auto error = el::path::PathError{el::path::PathErrorContext{"Path operation failed"_el}, cause};

        REQUIRE(error.hasCause());
        const auto text = toStdString(el::err::DiagnosticHelper{error}.toDocument().toString());
        WITH_CONTEXT(requireContains(text, "Caused By"));
        WITH_CONTEXT(requireContains(text, "Configuration lookup failed"));
    }

    void testStreamDiagnosticEmbedsPlatformContext() {
        const auto platform = std::make_shared<const el::system::PosixErrorContext>(EACCES, "Permission denied"_el);
        const auto error = el::stream::StreamError{
            el::stream::StreamErrorContext{"Stream could not be opened"_el, "The native open operation failed."_el}
                .setPath("/tmp/output.dat"_el)
                .setPlatformContext(platform)};

        REQUIRE_FALSE(error.hasCause());
        REQUIRE_EQUAL(error.title(), "Stream could not be opened"_el);
        REQUIRE_EQUAL(error.description(), "The native open operation failed."_el);
        REQUIRE_EQUAL(error.path(), "/tmp/output.dat"_el);
        REQUIRE_EQUAL(error.platformContext(), platform);
        REQUIRE_EQUAL(error.help(), "Check the stream permissions and access rights."_el);
        const auto diagnostic = error.diagnostic();
        REQUIRE_EQUAL(diagnostic->sourcePath(), "/tmp/output.dat"_el);
        const auto document = diagnostic->toTextDocument();
        const auto text = toStdString(document.toString());
        WITH_CONTEXT(requireContains(text, "Stream could not be opened"));
        WITH_CONTEXT(requireContains(text, "The native open operation failed."));
        WITH_CONTEXT(requireContains(text, "Check the stream permissions and access rights."));
        WITH_CONTEXT(requireContains(text, "/tmp/output.dat"));
        WITH_CONTEXT(requireContains(text, "Platform Error:"));
        WITH_CONTEXT(requireContains(text, "Permission denied"));
        REQUIRE(text.find("Caused By") == std::string::npos);
        REQUIRE(document.root()->contains(el::text::TextNodeType::Separator));
    }

    void testPosixCategoryMappingAndPlatformError() {
        const auto previousErrno = errno;
        errno = ENOENT;
        const auto capturedErrno = el::system::PosixErrorContext::fromErrno();
        errno = previousErrno;
        REQUIRE_EQUAL(capturedErrno->errorCode(), ENOENT);
        const auto context = el::system::PosixErrorContext::fromErrorCode(2);
        REQUIRE_EQUAL(context->category(), el::system::PlatformErrorCategory::NotFound);
        REQUIRE_EQUAL(
            el::system::PosixErrorContext{EACCES}.category(), el::system::PlatformErrorCategory::PermissionDenied);
        REQUIRE_EQUAL(
            el::system::PosixErrorContext{EEXIST}.category(), el::system::PlatformErrorCategory::AlreadyExists);
        REQUIRE_EQUAL(el::system::PosixErrorContext{-1}.category(), el::system::PlatformErrorCategory::Unknown);
        const auto error = el::system::PlatformError{"Native lookup failed"_el, context};
        REQUIRE_EQUAL(error.context(), context);
        const auto text = toStdString(error.diagnostic()->toTextDocument().toString());
        WITH_CONTEXT(requireContains(text, "Native lookup failed"));
        WITH_CONTEXT(requireContains(text, "errno"));
    }

    void testWindowsCategoryMappingAndUnknownFallback() {
#if defined(_WIN32)
        REQUIRE_EQUAL(
            el::system::WindowsErrorContext{ERROR_FILE_NOT_FOUND}.category(),
            el::system::PlatformErrorCategory::NotFound);
        REQUIRE_EQUAL(
            el::system::WindowsErrorContext{ERROR_ACCESS_DENIED}.category(),
            el::system::PlatformErrorCategory::PermissionDenied);
        REQUIRE_EQUAL(
            el::system::WindowsErrorContext{ERROR_ALREADY_EXISTS}.category(),
            el::system::PlatformErrorCategory::AlreadyExists);
        REQUIRE_EQUAL(
            el::system::WindowsErrorContext{0xffffffffU}.category(), el::system::PlatformErrorCategory::Unknown);
#endif
    }

    void testExternalPathAndSystemTextAreEscaped() {
        auto unsafePath = el::text::StringEditor{"/tmp/a"_el};
        unsafePath.append(U'\x1b').append("b"_el);
        auto unsafeMessage = el::text::StringEditor{"bad"_el};
        unsafeMessage.append(U'\n').append("message"_el);
        const auto platform = std::make_shared<const el::system::PosixErrorContext>(5, unsafeMessage);
        const auto error = el::path::PathError{el::path::PathErrorContext{"Path operation failed"_el}
                .setSourcePath(unsafePath)
                .setPlatformContext(platform)};
        const auto document = error.diagnostic()->toTextDocument();
        const auto text = toStdString(document.toString());

        REQUIRE(document.root()->contains(el::text::TextNodeType::EscapeSequence));
        REQUIRE(text.find('\x1b') == std::string::npos);
        REQUIRE(text.find("\\033") != std::string::npos);
        REQUIRE(text.find("bad\\nmessage") != std::string::npos);
    }

    void testForeignExceptionTextIsEscaped() {
        auto message = std::string{"foreign"};
        message.push_back('\x1b');
        message += "message";
        const auto error = std::runtime_error{message};
        const auto text = toStdString(el::err::DiagnosticHelper{error}.toDocument().toString());

        REQUIRE(text.find('\x1b') == std::string::npos);
        REQUIRE(text.find("foreign\\033message") != std::string::npos);
    }

    void testCauseHeadingUsesDisplayTextMap() {
        auto cause = std::make_exception_ptr(el::err::Exception{"Underlying failure"_el});
        auto error = el::err::Exception{"Top-level failure"_el, cause};
        auto displayText = el::i18n::DisplayTextMap::defaultMap()->clone();
        displayText->set("CausedByHeading"_el, "Reason Chain"_el);

        const auto text = toStdString(el::err::DiagnosticHelper{error, displayText}.toDocument().toString());

        WITH_CONTEXT(requireContains(text, "Reason Chain"));
        WITH_CONTEXT(requireContains(text, "Underlying failure"));
    }

    void testCauseHeadingPrecedesStyledBlockquote() {
        auto nested = std::make_exception_ptr(el::err::Exception{"Deep failure"_el});
        auto cause = std::make_exception_ptr(el::err::Exception{"Underlying failure"_el, nested});
        auto error = el::err::Exception{"Top-level failure"_el, cause};

        const auto document = el::err::DiagnosticHelper{error}.toDocument();
        const auto &rootChildren = document.root()->children();
        using NodeIndex = el::text::TextNodeList::Index;

        REQUIRE(rootChildren.count() >= el::text::TextNodeList::Count{3U});
        REQUIRE_EQUAL(rootChildren[NodeIndex{1U}]->type(), el::text::TextNodeType::Heading);
        REQUIRE(rootChildren[NodeIndex{1U}]->style().contains("diagnostic-cause"_el));
        REQUIRE_EQUAL(rootChildren[NodeIndex{2U}]->type(), el::text::TextNodeType::Blockquote);
        REQUIRE(rootChildren[NodeIndex{2U}]->style().contains("diagnostic-cause"_el));

        const auto &quoteChildren = rootChildren[NodeIndex{2U}]->children();
        REQUIRE(quoteChildren.count() >= el::text::TextNodeList::Count{3U});
        REQUIRE_EQUAL(quoteChildren[NodeIndex{1U}]->type(), el::text::TextNodeType::Heading);
        REQUIRE(quoteChildren[NodeIndex{1U}]->style().contains("diagnostic-cause"_el));
        REQUIRE_EQUAL(quoteChildren[NodeIndex{2U}]->type(), el::text::TextNodeType::Blockquote);
        REQUIRE(quoteChildren[NodeIndex{2U}]->style().contains("diagnostic-cause"_el));
    }

private:
    [[nodiscard]] static auto toStdString(const el::text::String &text) -> std::string {
        return el::text::StringConverter{text}.toStdString();
    }

    void requireContains(const std::string &text, const std::string &needle) {
        REQUIRE(text.find(needle) != std::string::npos);
    }
};
