// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/Application.hpp>
#include <erbsland/Path.hpp>
#include <erbsland/String.hpp>
#include <erbsland/re/CaptureRange.hpp>
#include <erbsland/re/RegEx.hpp>

#include <cstddef>
#include <vector>


namespace elgrep {

/// The application class for the recursive regular-expression search tool.
class ElGrepApp final : public el::Application {
    struct MatchingLine {
        std::size_t number;
        el::String text;
        std::vector<el::re::CaptureRange> ranges;
    };

    struct FileResult {
        el::String displayPath;
        std::size_t matchCount{0};
        std::vector<MatchingLine> lines;
    };

public:
    using Application::Application;

protected: // implement Application
    void initialize() override;
    void registerCommandLineOptions(const el::OptionsPtr &options) override;
    [[nodiscard]] auto main() -> el::ExitCode override;

private:
    [[nodiscard]] auto searchFile(
        const el::Path &path, const el::StringView &displayPath, const el::re::RegEx &expression) const -> FileResult;
    void searchDirectory(const el::Path &path, const el::re::RegEx &expression) const;
    void printResult(const FileResult &result) const;
    void printMatchingLine(const MatchingLine &line) const;
    void printText(const el::StringView &text) const;
};

}
