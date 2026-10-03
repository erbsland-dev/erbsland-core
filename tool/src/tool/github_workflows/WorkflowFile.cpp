// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "WorkflowFile.hpp"

#include <erbsland/ApplicationError.hpp>
#include <erbsland/CodeLocation.hpp>
#include <erbsland/re/Match.hpp>
#include <erbsland/RegEx.hpp>
#include <erbsland/StringEditor.hpp>
#include <erbsland/StringJoins.hpp>

#include <optional>

namespace tool::github_workflows {

using namespace el::text::literals;

WorkflowFile::WorkflowFile(el::Path path, const el::String &text) : _path{std::move(path)} {
    splitLines(text);
    parse();
}

auto WorkflowFile::isAction(const el::String &text) -> bool {
    static const auto cAction = el::RegEx::compile(R"([A-Za-z0-9_.\-]+/[A-Za-z0-9_.\-]+(?:/[A-Za-z0-9_.\-]+)*)"_el);
    if (!cAction->fullMatch(text)) {
        return false;
    }
    const auto parts = el::Path{text}.elements();
    return !parts.contains("."_el) && !parts.contains(".."_el);
}

auto WorkflowFile::isSha(const el::String &text) -> bool {
    static const auto cSha = el::RegEx::compile(R"([0-9a-fA-F]{40})"_el);
    return static_cast<bool>(cSha->fullMatch(text));
}

void WorkflowFile::splitLines(const el::String &text) {
    static const auto cLine = el::RegEx::compile(R"((?<text>[^\r\n]*)(?<ending>\r\n|\r|\n|$))"_el);
    for (const auto &match : cLine->findAll(text)) {
        if (match->content().isEmpty()) {
            continue;
        }
        _lines.append(WorkflowLine{match->content("text"_el), match->content("ending"_el)});
    }
}

void WorkflowFile::parse() {
    static const auto cUses = el::RegEx::compile(R"((?<indent>[ ]*)(?:-[ ]*)?uses:[ \t]*(?<value>.*))"_el);
    static const auto cScalar = el::RegEx::compile(R"((?:[^:#'"\n]+:|-[ ]*)[ \t]*[|>][1-9+\-]*[ \t]*(?:#.*)?)"_el);
    static const auto cUnsupported = el::RegEx::compile(R"([ \t]*(?:-[ \t]*)?(?:uses|"uses"|'uses')[ \t]*:.*)"_el);
    static const auto cFlow = el::RegEx::compile(R"((?:[^:#'"\n]+:)?[ \t]*(?:-[ \t]*)?[\[{].*)"_el);
    std::optional<el::ByteCount> scalarIndent;
    for (auto index = el::ItemIndex{}; index.isWithin(_lines.count()); ++index) {
        const auto line = _lines.get(index).text;
        const auto trimmed = line.trimmed();
        const auto content = line.trimmed(el::CharSet{U' '}, el::StringSide::Front);
        const auto indentation = line.length() - content.length();
        if (scalarIndent) {
            if (trimmed.isEmpty() || indentation > *scalarIndent) {
                continue;
            }
            scalarIndent.reset();
        }
        if (trimmed.isEmpty() || trimmed.startsWith("#"_el)) {
            continue;
        }
        if (cScalar->fullMatch(content)) {
            scalarIndent = indentation;
            if (!content.startsWith("uses:"_el) && !content.startsWith("- uses:"_el)) {
                continue;
            }
        }
        const auto match = cUses->fullMatch(line);
        if (!match) {
            if (cUnsupported->fullMatch(line)) {
                fail(index, "Only single-line block-style uses values are supported."_el);
            }
            if (cFlow->fullMatch(content) && containsFlowUses(content)) {
                fail(index, "Uses values in flow collections are not supported."_el);
            }
            continue;
        }
        auto value = match->content("value"_el);
        auto begin = el::ByteIndex::fromSizeT(match->begin("value"_el));
        if (value.startsWith("\""_el) || value.startsWith("'"_el)) {
            const auto quote = value.slice(el::StringSide::Front, el::ByteCount{1});
            const auto close = value.find(quote, el::ByteIndex{1});
            if (close.isNoIndex()) {
                fail(index, "The quoted uses value is not terminated on this line."_el);
            }
            const auto suffix = value.slice(el::ByteRange::fromBegin(close + el::ByteCount{1})).trimmed();
            if (!suffix.isEmpty() && !suffix.startsWith("#"_el)) {
                fail(index, "Unexpected content after the quoted uses value."_el);
            }
            value = value.slice(el::ByteRange{el::ByteIndex{1}, close});
            ++begin;
        } else {
            const auto separator = value.findFirstOf(el::CharSet{U' ', U'\t'});
            if (!separator.isNoIndex()) {
                const auto suffix = value.slice(el::ByteRange::fromBegin(separator)).trimmed();
                if (!suffix.isEmpty() && !suffix.startsWith("#"_el)) {
                    fail(index, "Unexpected content after the unquoted uses value."_el);
                }
                value = value.slice(el::ByteRange{el::ByteIndex{}, separator});
            }
        }
        if (value.startsWith("./"_el) || value.startsWith("docker://"_el)) {
            continue;
        }
        const auto pair = value.splitAtLast("@"_el);
        auto action = value;
        auto ref = el::String{};
        if (pair) {
            action = pair->first;
            ref = pair->second;
        }
        if (!isAction(action) || ref.containsOneOf(el::CharSet{U' ', U'\t'})) {
            fail(index, "Expected owner/repository[/subpath]@reference as a plain single-line value."_el);
        }
        const auto slash = action.find("/"_el, action.find("/"_el) + el::ByteCount{1});
        auto repository = action;
        if (!slash.isNoIndex()) {
            repository = action.slice(el::ByteRange{el::ByteIndex{}, slash});
        }
        _references.append(
            ActionReference{
                index, el::ByteRange{begin, value.length()}, match->content("indent"_el), action, repository, ref});
    }
}

auto WorkflowFile::containsFlowUses(const el::String &text) -> bool {
    static const auto cKey = el::RegEx::compile(R"([ \t]*(?:uses|"uses"|'uses')[ \t]*:.*)"_el);
    auto quote = el::Char{};
    auto position = el::ByteIndex{};
    while (position.isWithin(text.length())) {
        const auto character = text.readCharAndAdvance(position);
        if (quote != el::Char{}) {
            if (quote == U'"' && character == U'\\') {
                text.advance(position);
            } else if (character == quote) {
                if (quote == U'\'' && text.charAt(position) == U'\'') {
                    text.advance(position);
                } else {
                    quote = {};
                }
            }
        } else if (character == U'#') {
            return false;
        } else if (character == U'"' || character == U'\'') {
            quote = character;
        } else if (
            (character == U'{' || character == U',') &&
            cKey->fullMatch(text.slice(el::ByteRange::fromBegin(position)))) {
            return true;
        }
    }
    return false;
}

auto WorkflowFile::annotation(const ActionReference &reference, const el::String &version) -> el::String {
    return el::join(reference.indent, "# "_el, reference.action, "@"_el, version);
}

auto WorkflowFile::annotationLine(const ActionReference &reference) const -> el::ItemIndex {
    if (reference.line == el::ItemIndex{}) {
        return el::ItemIndex::noIndex();
    }
    const auto previous = reference.line - el::ItemCount{1};
    const auto line = _lines.get(previous).text;
    static const auto cAnnotation =
        el::RegEx::compile(R"([ ]*#[ ]+[A-Za-z0-9_.\-/]+@v[0-9]+\.[0-9]+\.[0-9]+[ \t]*)"_el);
    if (line.startsWith(el::join(reference.indent, "#"_el)) && cAnnotation->fullMatch(line)) {
        return previous;
    }
    return el::ItemIndex::noIndex();
}

auto WorkflowFile::hasAnnotation(const ActionReference &reference, const el::String &version) const -> bool {
    const auto previous = annotationLine(reference);
    return previous.isValid() && _lines.get(previous).text == annotation(reference, version);
}

void WorkflowFile::repair(const ActionReference &reference, const el::String &sha, const el::String &version) {
    if (!sha.isEmpty()) {
        const auto line = _lines.get(reference.line).text;
        _replacements.set(
            reference.line,
            el::join(
                line.slice(el::ByteRange{el::ByteIndex{}, reference.valueRange.beginIndex()}),
                reference.action,
                "@"_el,
                sha,
                line.slice(el::ByteRange::fromBegin(reference.valueRange.endIndex()))));
    }
    const auto previous = annotationLine(reference);
    if (previous.isValid()) {
        _replacements.set(previous, annotation(reference, version));
    } else {
        _insertions.set(reference.line, annotation(reference, version));
    }
}

auto WorkflowFile::text() const -> el::String {
    auto result = el::StringEditor{};
    auto defaultEnding = el::String{"\n"_el};
    for (const auto &line : _lines) {
        if (!line.ending.isEmpty()) {
            defaultEnding = line.ending;
            break;
        }
    }
    for (auto index = el::ItemIndex{}; index.isWithin(_lines.count()); ++index) {
        const auto line = _lines.get(index);
        if (_insertions.contains(index)) {
            auto ending = line.ending;
            if (ending.isEmpty()) {
                ending = defaultEnding;
            }
            result.append(_insertions.get(index, {})).append(ending);
        }
        result.append(_replacements.get(index, line.text)).append(line.ending);
    }
    return result;
}

void WorkflowFile::fail(const el::ItemIndex line, const el::String &description) const {
    throw el::ApplicationError{el::ApplicationErrorContext{}
            .setTitle("Unsupported Workflow Reference"_el)
            .setDescription(description)
            .setSourcePath(_path.toString())
            .setCodeLocation(el::CodeLocation{el::LineIndex::fromInteger(line.toRawValue()), el::ColumnIndex{}})};
}

}
