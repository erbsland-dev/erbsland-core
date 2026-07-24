// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../ConfTestHelper.hpp"

#include <erbsland/conf/Parser.hpp>
#include <erbsland/conf/StdFormat.hpp>
#include <erbsland/mem/impl/UnsafeByteBlockAccess.hpp>
#include <erbsland/text/StringList.hpp>

#include <algorithm>
#include <filesystem>
#include <map>

using namespace el::conf;

class ParserTestHelper : public ConfTestHelper {
public:
    using ExpectedValueMap = std::map<el::text::String, el::text::String>;

    class MockSource final : public Source {
    public:
        MockSource() = default;
        ~MockSource() override = default;
        [[nodiscard]] auto identifier() const noexcept -> SourceIdentifierPtr override {
            return SourceIdentifier::createForFile("mock.elcl"_el);
        }
        void open() override {
            _open = true;
            actions.append("open"_el);
        }
        [[nodiscard]] auto isOpen() const noexcept -> bool override { return _open; }
        [[nodiscard]] auto atEnd() const noexcept -> bool override { return currentLine >= lines.size(); }
        [[nodiscard]] auto readLine() -> el::text::String override {
            if (atEnd()) {
                return {};
            }
            el::text::String line;
            if (std::holds_alternative<el::text::String>(lines[currentLine])) {
                line = std::get<el::text::String>(lines[currentLine]);
            } else {
                const auto &bytes = std::get<el::mem::ByteBlock>(lines[currentLine]);
                const auto byteSpan = el::mem::impl::UnsafeByteBlockAccess{bytes}.data();
                line = el::text::String{
                    std::string_view{reinterpret_cast<const char *>(byteSpan.data()), byteSpan.size()}};
            }
            currentLine += 1;
            actions.append("readLine"_el);
            return line;
        }
        [[nodiscard]] auto codeSnippet(el::unit::CodeLocation) noexcept
            -> std::optional<el::text::CodeSnippet> override {
            return std::nullopt;
        }
        void close() noexcept override {
            _open = false;
            actions.append("close"_el);
        }

        using Line = std::variant<el::text::String, el::mem::ByteBlock>;

        std::size_t currentLine = 0;
        std::vector<Line> lines;
        el::text::StringList actions;

        bool _open = false;
    };
    using MockSourcePtr = std::shared_ptr<MockSource>;

public:
    void verifyValueMap(const ExpectedValueMap &expectedValueMap) {
        auto flatMap = doc->toFlatValueMap();
        // First, convert and verify all name paths.
        auto actualValues = std::map<el::text::String, el::text::String>{};
        for (auto it = flatMap.begin(); it != flatMap.end(); ++it) {
            if (it->second->type() == ValueType::Document) {
                continue; // ignore the document itself.
            }
            auto namePathText = it->first.toText();
            runWithContext(
                SOURCE_LOCATION(),
                [&]() { REQUIRE(expectedValueMap.contains(namePathText)); },
                [&]() -> std::string {
                    return std::format(
                        "Unexpected additional value: {} = {}",
                        el::text::StringConverter{namePathText}.toStdString(),
                        el::text::StringConverter{it->second->toTestText()}.toStdString());
                });
            actualValues[namePathText] = it->second->toTestText();
        }
        // Now test if all expected values are part of the document.
        for (const auto &[expectedNamePath, expectedValueText] : expectedValueMap) {
            runWithContext(
                SOURCE_LOCATION(),
                [&]() { REQUIRE(actualValues.contains(expectedNamePath)); },
                [&]() -> std::string {
                    return std::format(
                        "Missing value: {} = {}",
                        el::text::StringConverter{expectedNamePath}.toStdString(),
                        el::text::StringConverter{expectedValueText}.toStdString());
                });
            const auto actualValueText = actualValues[expectedNamePath];
            if (expectedValueText.startsWith("Float("_el)) {
                // special handling for floating point values.
                const auto expectedText = el::text::StringConverter{expectedValueText}.toStdString();
                const auto expectedFloat = std::stof(expectedText.substr(6, expectedText.size() - 7));
                REQUIRE(actualValueText.startsWith("Float("_el));
                const auto actualText = el::text::StringConverter{actualValueText}.toStdString();
                const auto actualFloat = std::stof(actualText.substr(6, actualText.size() - 7));
                REQUIRE_LESS(std::abs(actualFloat - expectedFloat), std::numeric_limits<double>::epsilon());
            } else {
                REQUIRE_EQUAL(actualValueText, expectedValueText);
            }
        }
    }

    auto createTestFile(const std::filesystem::path &relativePath, const el::text::String &text)
        -> std::filesystem::path {

        if (!relativePath.is_relative()) {
            throw std::logic_error("The path must be relative.");
        }
        const auto filePath = useTestFileDirectory() / relativePath;
        std::filesystem::create_directories(filePath.parent_path());
        std::ofstream stream(filePath, std::ios::binary);
        stream << el::text::StringConverter{text}.toStdString();
        stream.close();
        return filePath;
    }

public:
    DocumentPtr doc; ///< Document instance.
};
