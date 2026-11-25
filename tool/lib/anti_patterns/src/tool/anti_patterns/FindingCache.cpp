// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "FindingCache.hpp"

#include <erbsland/ByteFormat.hpp>
#include <erbsland/json/JsonValue.hpp>
#include <erbsland/PathInfo.hpp>
#include <erbsland/PathOperations.hpp>
#include <erbsland/StringFormat.hpp>
#include <erbsland/StringJoins.hpp>

#include <cstddef>
#include <utility>

namespace tool::anti_patterns {

using namespace el::text::literals;

using el::data::json::JsonArray;
using el::data::json::JsonObject;
using el::data::json::JsonValue;

FindingCache::FindingCache(const frame::ProjectFiles &files, const RuleRegistry &rules) :
    _files{files}, _rules{rules}, _binarySignature{binarySignature()} {
    load();
}

auto FindingCache::signature(const el::Path &path) -> el::String {
    auto info = path.info();
    info.reload();
    return signature(info);
}

auto FindingCache::signature(const el::PathInfo &info) -> el::String {
    if (!info.isRegularFile()) {
        return {};
    }
    const auto modified = info.lastModified().toRawValue();
    const auto changed = info.lastMetadataChange().toRawValue();
    const auto created = info.creationTime().toRawValue();
    return el::StringFormat{"{}:{}:{}:{}:{}:{}:{}"_el}.build(
        info.fileSize().toRawValue(),
        modified.first,
        modified.second,
        changed.first,
        changed.second,
        created.first,
        created.second);
}

auto FindingCache::binarySignature() const -> el::String {
    const auto executable = el::Path::executablePath();
    if (executable.isEmpty()) {
        return {};
    }
    return signature(executable);
}

void FindingCache::load() {
    try {
        const auto path = _files.checked(el::Path{".cache/anti_patterns.json"_el});
        if (!path.info().exists() || path.info().fileSize() > el::ByteCount{32U * 1024U * 1024U}) {
            return;
        }
        const auto parsed = JsonValue::fromString(_files.readText(path));
        const auto configuration =
            el::String::fromByteBlock(_rules.configurationSignature(), el::ByteFormat::compact());
        if (!parsed || parsed->get(cVersionField).get<int64_t>(0) != 5 ||
            parsed->get(cBinaryField).getText() != _binarySignature ||
            parsed->get(cConfigurationField).getText() != configuration) {
            _dirty = true;
            return;
        }
        const auto entries = parsed->getOrThrow(cFilesField);
        for (auto index = el::ItemIndex::zero(); index.isWithin(entries.itemCount()); ++index) {
            const auto entry = entries.getOrThrow(index);
            const auto pathText = entry.getOrThrow(cPathField).getTextOrThrow();
            auto record = std::make_shared<Record>();
            record->signature = entry.getOrThrow(cSignatureField).getTextOrThrow();
            const auto saved = entry.getOrThrow(cFindingsField);
            for (auto findingIndex = el::ItemIndex::zero(); findingIndex.isWithin(saved.itemCount()); ++findingIndex) {
                const auto value = saved.getOrThrow(findingIndex);
                auto finding = Finding{};
                finding.rule = value.getOrThrow(cRuleField).getTextOrThrow();
                const auto *rule = _rules.find(finding.rule);
                if (rule == nullptr) {
                    throw std::runtime_error{"Unknown cached rule"};
                }
                finding.severity = rule->severity();
                finding.path = el::Path{pathText};
                const auto start = value.getOrThrow(cStartField).getOrThrow<int64_t>();
                const auto end = value.getOrThrow(cEndField).getOrThrow<int64_t>();
                const auto line = value.getOrThrow(cLineField).getOrThrow<int64_t>();
                const auto endLine = value.getOrThrow(cEndLineField).getOrThrow<int64_t>();
                if (start < 0 || end <= start || line < 1 || endLine < line) {
                    throw std::runtime_error{"Invalid cached finding"};
                }
                finding.range = el::ByteRange{
                    el::ByteIndex::fromInteger(static_cast<el::ByteIndex::Value>(start)),
                    el::ByteIndex::fromInteger(static_cast<el::ByteIndex::Value>(end))};
                finding.lines = el::LineRange{
                    el::LineIndex::fromSizeT(static_cast<std::size_t>(line - 1)),
                    el::LineIndex::fromSizeT(static_cast<std::size_t>(endLine))};
                finding.snippet = value.getOrThrow(cSnippetField).getTextOrThrow();
                finding.inlineReason = value.getOrThrow(cInlineReasonField).getTextOrThrow();
                record->findings.append(std::move(finding));
            }
            const auto inserted = _records.tryInsert(pathText, std::move(record));
            (void)inserted;
        }
    } catch (const std::exception &) {
        _records.clear();
        _dirty = true;
    }
}

auto FindingCache::get(const el::Path &path, const el::String &signature) const -> const el::List<Finding> * {
    const auto record = _records.get(path.toString(), nullptr);
    if (!record || record->signature != signature) {
        return nullptr;
    }
    return &record->findings;
}

void FindingCache::put(const el::Path &path, el::String signature, el::List<Finding> findings) {
    _records.set(path.toString(), std::make_shared<Record>(Record{std::move(signature), std::move(findings)}));
    _dirty = true;
}

void FindingCache::retain(const el::StringSet &paths) {
    const auto previousCount = _records.count();
    _records.removeIfKey([&](const el::String &path) -> bool { return !paths.contains(path); });
    if (_records.count() != previousCount) {
        _dirty = true;
    }
}

void FindingCache::save() const {
    if (!_dirty) {
        return;
    }
    try {
        auto root = JsonValue{JsonObject{}};
        root.set(cVersionField, 5)
            .set(cBinaryField, _binarySignature)
            .set(
                cConfigurationField,
                el::String::fromByteBlock(_rules.configurationSignature(), el::ByteFormat::compact()));
        auto files = JsonValue{JsonArray{}};
        for (const auto &[path, record] : _records) {
            auto entry = JsonValue{JsonObject{}};
            entry.set(cPathField, path).set(cSignatureField, record->signature);
            auto findings = JsonValue{JsonArray{}};
            for (const auto &finding : record->findings) {
                auto value = JsonValue{JsonObject{}};
                value.set(cRuleField, finding.rule)
                    .set(cStartField, finding.range.beginIndex().toRawValue())
                    .set(cEndField, finding.range.endIndex().toRawValue())
                    .set(cLineField, finding.lineNumber())
                    .set(cEndLineField, finding.endLineNumber())
                    .set(cSnippetField, finding.snippet)
                    .set(cInlineReasonField, finding.inlineReason);
                findings.append(std::move(value));
            }
            entry.set(cFindingsField, std::move(findings));
            files.append(std::move(entry));
        }
        root.set(cFilesField, std::move(files));
        const auto path = _files.checked(el::Path{".cache/anti_patterns.json"_el}, true);
        if (!path.parent().info().isDirectory()) {
            path.parent().operations().createDirectoryOrThrow(el::PathCreateDirectoryOptions{}.setCreateParents(true));
        }
        _files.updateText(path, el::join(root.toString(), "\n"_el));
    } catch (const std::exception &) {
        // A disposable cache must never determine scan success.
    }
}

}
