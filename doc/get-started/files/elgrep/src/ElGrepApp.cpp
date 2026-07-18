// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ElGrepApp.hpp"

#include <erbsland/all_cterm.hpp>
#include <erbsland/all_options.hpp>
#include <erbsland/all_path.hpp>
#include <erbsland/all_re.hpp>
#include <erbsland/all_stream.hpp>
#include <erbsland/all_text.hpp>
#include <erbsland/ApplicationError.hpp>

namespace elgrep {

using namespace el::text::literals;
namespace ct = el::cterm;

void ElGrepApp::initialize() {
    info().setApplicationName("elgrep"_el);
    info().setApplicationVersion(el::Version{0, 1, 0});
    info().setAuthorName("Erbsland Core tutorial"_el);
    info().setLicenseText("Apache-2.0"_el);
}

void ElGrepApp::registerCommandLineOptions(const el::OptionsPtr &options) {
    options->setHelpTitle("elgrep - search text with a regular expression"_el);
    options->setHelpDescription(
        "Searches a UTF-8 text file, or recursively searches all regular files below a directory."_el);
    options->addOption({"-r"_el, "--recursive"_el, "recursive"_el})
        .setType(el::OptionType::Flag)
        .setHelpDescription("Recursively searches a directory and skips symbolic links."_el);
    options->addOption("path"_el).setRequired().setHelpDescription("File or directory to search."_el);
    options->addOption("pattern"_el).setRequired().setHelpDescription("Erbsland regular-expression pattern."_el);
}

auto ElGrepApp::main() -> el::ExitCode {
    const auto values = optionValues();
    const auto path = el::Path::fromNativeOrThrow(values->getText("path"_el));
    const auto pathInfo = path.info(el::PathInfoPart::Type);
    if (!pathInfo.exists()) {
        throw el::ApplicationError{"The search path does not exist or cannot be accessed."_el};
    }

    const auto expression = el::re::RegEx::compile(values->getText("pattern"_el));
    const auto resolvedPath = pathInfo.resolvedPath();
    if (pathInfo.isRegularFile()) {
        printResult(searchFile(resolvedPath, resolvedPath.name(), *expression));
        return el::ExitCode::success();
    }
    if (!pathInfo.isDirectory()) {
        throw el::ApplicationError{"The search path is neither a regular file nor a directory."_el};
    }
    if (!values->getFlag("recursive"_el)) {
        throw el::ApplicationError{"The search path is a directory. Add --recursive to search it."_el};
    }

    searchDirectory(resolvedPath, *expression);
    return el::ExitCode::success();
}

auto ElGrepApp::searchFile(
    const el::Path &path, const el::String &displayPath, const el::re::RegEx &expression) const -> FileResult {
    auto result = FileResult{.displayPath = displayPath};
    const auto input = path.content().openTextInputStream();
    const auto lineEndCharacters = el::CharSet{"\r\n"_el};
    auto lineNumber = std::size_t{1};

    while (true) {
        const auto readResult = input->readLine();
        if (readResult.isFinished()) {
            break;
        }
        if (readResult.isTimeout()) {
            throw el::ApplicationError{"Reading a searched file timed out."_el};
        }

        auto line = readResult.data().trimmed(lineEndCharacters, el::StringSide::Back);
        const auto matches = expression.collectAll(line);
        if (!matches.empty()) {
            auto matchingLine = MatchingLine{.number = lineNumber, .text = std::move(line)};
            matchingLine.ranges.reserve(matches.size());
            for (const auto &match : matches) {
                matchingLine.ranges.push_back(match->range());
            }
            result.matchCount += matches.size();
            result.lines.push_back(std::move(matchingLine));
        }
        ++lineNumber;
    }
    return result;
}

void ElGrepApp::searchDirectory(const el::Path &path, const el::re::RegEx &expression) const {
    auto walkOptions = el::PathWalkOptions{};
    walkOptions.setTypes(el::PathType::RegularFile).setSymlinkMode(el::SymlinkMode::Skip);
    path.walker().walkOrThrow(
        [&](const el::Path &filePath) -> el::PathWalkStatus {
            const auto displayPath = filePath.toRelativeOrThrow(path).toString();
            printResult(searchFile(filePath, displayPath, expression));
            return el::PathWalkStatus::Continue;
        },
        walkOptions);
}

void ElGrepApp::printResult(const FileResult &result) const {
    terminal()->print(ct::fg::BrightCyan);
    printText(result.displayPath);
    terminal()->printLine(
        ct::fg::Default,
        ": "_el,
        ct::fg::BrightGreen,
        el::String::fromInteger(result.matchCount),
        ct::fg::Default,
        result.matchCount == 1 ? " match"_el : " matches"_el);
    for (const auto &line : result.lines) {
        printMatchingLine(line);
    }
}

void ElGrepApp::printMatchingLine(const MatchingLine &line) const {
    terminal()->print(ct::fg::BrightBlack, "    "_el, el::String::fromInteger(line.number), ": "_el, ct::fg::Default);

    auto lastEnd = std::size_t{0};
    for (const auto &range : line.ranges) {
        printText(
            line.text.slice(el::ByteRange{el::ByteIndex::fromSizeT(lastEnd), el::ByteIndex::fromSizeT(range.begin())}));
        terminal()->print(ct::fg::BrightYellow, ct::BlockAttributes{ct::BlockAttributes::Bold});
        if (range.isEmpty()) {
            terminal()->print("▏"_el);
        } else {
            printText(line.text.slice(
                el::ByteRange{el::ByteIndex::fromSizeT(range.begin()), el::ByteIndex::fromSizeT(range.end())}));
        }
        terminal()->print(ct::Color::reset(), ct::BlockAttributes::reset());
        lastEnd = range.end();
    }
    printText(line.text.slice(el::StringSide::Back, el::ByteIndex::fromSizeT(lastEnd)));
    terminal()->writeLineBreak();
}

void ElGrepApp::printText(const el::String &text) const {
    terminal()->print(text.toEscaped(el::EscapeFormat::Display));
}

}
