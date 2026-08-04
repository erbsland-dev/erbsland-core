// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "CryptologyResponseReader.hpp"

#include <erbsland/err/Exception.hpp>
#include <erbsland/err/RuntimeError.hpp>
#include <erbsland/path/PathContent.hpp>
#include <erbsland/path/PathInfo.hpp>
#include <erbsland/text/CharSet.hpp>
#include <erbsland/text/IntegerBase.hpp>
#include <erbsland/text/IntegerParseOptions.hpp>
#include <erbsland/text/StringFormat.hpp>
#include <erbsland/text/StringList.hpp>
#include <erbsland/unittest/FileHelper.hpp>

#include <algorithm>

using namespace el::text::literals;

auto CryptologyResponseReader::Record::hasValue(const el::String &name) const -> bool {
    return values.contains(name);
}

auto CryptologyResponseReader::Record::value(const el::String &name) const -> el::String {
    return required(values, name, "response value"_el);
}

auto CryptologyResponseReader::Record::setting(const el::String &name) const -> el::String {
    return required(group, name, "response setting"_el);
}

auto CryptologyResponseReader::Record::unsignedValue(const el::String &name) const -> std::size_t {
    return unsignedInteger(value(name));
}

auto CryptologyResponseReader::Record::unsignedSetting(const el::String &name) const -> std::size_t {
    return unsignedInteger(setting(name));
}

void CryptologyResponseReader::Record::requireValues(const std::initializer_list<el::String> names) const {
    for (const auto &name : names) {
        static_cast<void>(value(name));
    }
}

void CryptologyResponseReader::Record::requireAllowedValues(const std::initializer_list<el::String> names) const {
    requireAllowed(values, names, "response value"_el);
}

void CryptologyResponseReader::Record::requireAllowedSettings(const std::initializer_list<el::String> names) const {
    requireAllowed(group, names, "response setting"_el);
}

auto CryptologyResponseReader::Record::diagnostic() const -> el::String {
    auto directionText = "none"_el;
    if (direction == Direction::Encrypt) {
        directionText = "encrypt"_el;
    } else if (direction == Direction::Decrypt) {
        directionText = "decrypt"_el;
    }
    return el::StringFormat{"{} line {} record {} direction {}"_el}.build(path.toString(), line, index, directionText);
}

auto CryptologyResponseReader::Record::required(
    const Values &source, const el::String &name, const el::String &kind) const -> el::String {
    if (const auto result = source.get(name); result.has_value()) {
        return *result;
    }
    throw el::RuntimeError{el::StringFormat{"Missing {} '{}' in {}."_el}.build(kind, name, diagnostic())};
}

auto CryptologyResponseReader::Record::unsignedInteger(const el::String &text) const -> std::size_t {
    try {
        auto options = el::IntegerParseOptions::parserDefault();
        options.setFixedBase(el::IntegerBase::Decimal);
        return text.toIntegerOrThrow<std::size_t>(options);
    } catch (const el::Exception &) {
        throw el::RuntimeError{el::StringFormat{"Invalid unsigned integer in {}."_el}.build(diagnostic())};
    }
}

void CryptologyResponseReader::Record::requireAllowed(
    const Values &source, const std::initializer_list<el::String> names, const el::String &kind) const {
    for (const auto &[name, value] : source) {
        static_cast<void>(value);
        if (std::ranges::find(names, name) == names.end()) {
            throw el::RuntimeError{el::StringFormat{"Unknown {} '{}' in {}."_el}.build(kind, name, diagnostic())};
        }
    }
}

CryptologyResponseReader::CryptologyResponseReader(const el::Path &relativePath) :
    _path{resolveDataPath(relativePath)} {
}

auto CryptologyResponseReader::read() const -> std::vector<Record> {
    const auto content = _path.content().readTextOrThrow();
    const auto lines = el::StringList::fromSplit(content, el::CharSet{U'\n'}, el::ItemCount::infinite(), true);
    auto records = std::vector<Record>{};
    auto group = Record::Values{};
    auto direction = Direction::None;
    auto values = Record::Values{};
    auto failed = false;
    auto firstLine = std::size_t{};

    const auto finishRecord = [&]() -> void {
        if (values.count().isZero() && !failed) {
            return;
        }
        records.push_back(
            Record{
                .path = _path,
                .line = firstLine,
                .index = records.size(),
                .direction = direction,
                .group = group,
                .values = std::move(values),
                .failed = failed,
            });
        values = {};
        failed = false;
        firstLine = 0U;
    };

    auto lineNumber = std::size_t{};
    for (const auto &sourceLine : lines) {
        ++lineNumber;
        const auto line = sourceLine.trimmed();
        if (line.isEmpty()) {
            finishRecord();
            continue;
        }
        if (line.startsWith("#"_el)) {
            finishRecord();
            continue;
        }
        if (line == "[ENCRYPT]"_el) {
            finishRecord();
            direction = Direction::Encrypt;
            continue;
        }
        if (line == "[DECRYPT]"_el) {
            finishRecord();
            direction = Direction::Decrypt;
            continue;
        }
        if (line.charAt(el::StringSide::Front) == U'[' && line.charAt(el::StringSide::Back) == U']') {
            finishRecord();
            const auto groupLine = line.slice(el::ByteRange{el::ByteIndex{1U}, line.length() - el::ByteLength{2U}});
            const auto [name, value] = assignment(groupLine, lineNumber);
            group.set(name, value);
            continue;
        }
        if (line == "FAIL"_el) {
            if (firstLine == 0U) {
                firstLine = lineNumber;
            }
            failed = true;
            continue;
        }
        const auto [name, value] = assignment(line, lineNumber);
        if (firstLine == 0U) {
            firstLine = lineNumber;
        }
        if (!values.tryInsert(name, value)) {
            throw el::RuntimeError{el::StringFormat{"Duplicate response value '{}' in {} line {}."_el}.build(
                name, _path.toString(), lineNumber)};
        }
    }
    finishRecord();
    return records;
}

auto CryptologyResponseReader::resolveDataPath(const el::Path &relativePath) -> el::Path {
    const auto executableDirectory = el::Path{el::unittest::fh::unitTestExecutablePath()}.parent();
    const auto directPath = executableDirectory / relativePath;
    if (directPath.info().exists()) {
        return directPath;
    }
    const auto parentPath = executableDirectory.parent() / relativePath;
    if (parentPath.info().exists()) {
        return parentPath;
    }
    throw el::RuntimeError{el::StringFormat{"Could not resolve test data path '{}'. Tried '{}' and '{}'."_el}.build(
        relativePath.toString(), directPath.toString(), parentPath.toString())};
}

auto CryptologyResponseReader::assignment(const el::String &line, const std::size_t lineNumber) const
    -> std::pair<el::String, el::String> {
    const auto parts = el::StringList::fromSplit(line, el::CharSet{U'='}, el::ItemCount{1U}, true);
    if (parts.count() != el::ItemCount{2U}) {
        throw el::RuntimeError{
            el::StringFormat{"Malformed response line in {} line {}."_el}.build(_path.toString(), lineNumber)};
    }
    const auto name = parts.get(el::ItemIndex{0U}).trimmed();
    if (name.isEmpty()) {
        throw el::RuntimeError{
            el::StringFormat{"Empty response name in {} line {}."_el}.build(_path.toString(), lineNumber)};
    }
    return {name, parts.get(el::ItemIndex{1U}).trimmed()};
}
