// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TestHelper.hpp"

#include <erbsland/text/StringConverter.hpp>

#include <format>
#include <stdexcept>
#include <string>
#include <vector>

class BlockStringTestHelper : public TestHelper {
public:
    [[nodiscard]] auto render(const BlockString &text) -> std::string {
        auto result = std::string{};
        for (const auto &character : text) {
            result += blockToStdString(character);
        }
        return result;
    }

    [[nodiscard]] auto render(const BlockStringView text) -> std::string {
        auto result = std::string{};
        for (const auto &character : text) {
            result += blockToStdString(character);
        }
        return result;
    }

    [[nodiscard]] auto render(const std::u32string &text) -> std::string {
        auto result = std::string{};
        for (const auto character : text) {
            result += blockToStdString(Block{character});
        }
        return result;
    }

    [[nodiscard]] auto renderLines(const BlockStringLines &lines) -> std::vector<std::string> {
        auto result = std::vector<std::string>{};
        result.reserve(lines.size());
        for (const auto &line : lines) {
            result.push_back(render(line));
        }
        return result;
    }

    [[nodiscard]] auto renderWords(const std::vector<BlockString> &words) -> std::vector<std::string> {
        auto result = std::vector<std::string>{};
        result.reserve(words.size());
        for (const auto &word : words) {
            result.push_back(render(word));
        }
        return result;
    }

    [[nodiscard]] auto renderWords(const std::vector<BlockStringView> &words) -> std::vector<std::string> {
        auto result = std::vector<std::string>{};
        result.reserve(words.size());
        for (const auto word : words) {
            result.push_back(render(word));
        }
        return result;
    }

    [[nodiscard]] auto toPlainText(const BlockString &text) -> std::string {
        auto result = std::string{};
        for (const auto &character : text) {
            result += erbsland::text::StringConverter{character.charStr()}.toStdString();
        }
        return result;
    }

    void requireStringEqual(const BlockString &actual, const std::u32string_view expected) {
        if (actual.length().toSizeT() != expected.size()) {
            throw std::runtime_error(
                std::format(
                    "BlockString length mismatch: actual={} expected={}", actual.length().toSizeT(), expected.size()));
        }
        for (std::size_t index = 0; index < expected.size(); ++index) {
            if (actual[BlockIndex::fromSizeT(index)] != expected[index]) {
                throw std::runtime_error(std::format("BlockString mismatch at index {}", index));
            }
        }
    }
};
