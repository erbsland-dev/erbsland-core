// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "MissingApiDocumentationRule.hpp"

#include "../impl/RuleSupport.hpp"
#include "../SourceIndex.hpp"

#include <erbsland/re/Match.hpp>
#include <erbsland/RegEx.hpp>
#include <erbsland/StringJoins.hpp>

namespace tool::anti_patterns::rules {

using namespace el::text::literals;

auto MissingApiDocumentationRule::create() -> RulePtr {
    return std::make_unique<MissingApiDocumentationRule>();
}

auto MissingApiDocumentationRule::identifier() const -> el::String {
    return "missing_api_documentation"_el;
}

auto MissingApiDocumentationRule::title() const -> el::String {
    return "Missing API documentation"_el;
}

auto MissingApiDocumentationRule::description() const -> el::String {
    return "Detect API declarations that lack required documentation."_el;
}

auto MissingApiDocumentationRule::severity() const -> Severity {
    return Severity::Medium;
}

void MissingApiDocumentationRule::addConfigurationRules(el::conf::vr::RulesBuilder &builder) const {
    using el::conf::vr::RuleType;
    using namespace el::conf::vr::builder;
    builder.addRule(
        cFileExemptionTextsConfig,
        RuleType::ValueList,
        EntryType{RuleType::Text},
        EntryConstraints{Matches{R"(\A(?:\s|\S)*\S(?:\s|\S)*\z)"_el}},
        IsOptional{},
        Title{"File documentation exemptions"_el},
        Description{"Case-sensitive class or struct header substrings that exempt the complete file."_el});
    builder.addRule(
        cDeclarationExemptionTextsConfig,
        RuleType::ValueList,
        EntryType{RuleType::Text},
        EntryConstraints{Matches{R"(\A(?:\s|\S)*\S(?:\s|\S)*\z)"_el}},
        IsOptional{},
        Title{"Declaration documentation exemptions"_el},
        Description{"Case-sensitive declaration substrings that exempt individual API declarations."_el});
}

void MissingApiDocumentationRule::configure(const el::conf::ValuePtr &configuration) {
    if (configuration == nullptr) {
        _fileExemptionTexts.clear();
        _declarationExemptionTexts.clear();
        return;
    }
    _fileExemptionTexts = configuration->getStringList(cFileExemptionTextsConfig);
    _declarationExemptionTexts = configuration->getStringList(cDeclarationExemptionTextsConfig);
}

auto MissingApiDocumentationRule::isFileExempt(const SourceIndex &source) const -> bool {
    if (source.path().suffix() != ".hpp"_el || source.path().name().endsWith("_fwd.hpp"_el)) {
        return true;
    }
    for (const auto &block : source.blocks()) {
        if (block.kind == SourceBlock::Kind::Class || block.kind == SourceBlock::Kind::Struct) {
            const auto header = source.slice(el::ByteRange{block.headerStart, block.openingBrace()}, true);
            for (const auto &text : _fileExemptionTexts) {
                if (header.contains(text)) {
                    return true;
                }
            }
        }
    }
    return false;
}

auto MissingApiDocumentationRule::findApiScope(const SourceIndex &source, const el::ByteIndex start)
    -> std::optional<ApiScope> {
    auto current = source.enclosingBlock(start);
    auto scope = ApiScope{};
    while (current.isValid()) {
        const auto &block = source.blocks().getRef(current);
        if (block.kind != SourceBlock::Kind::Namespace && block.kind != SourceBlock::Kind::Class &&
            block.kind != SourceBlock::Kind::Struct) {
            return std::nullopt;
        }
        if (scope.typeIndex.isNoIndex() &&
            (block.kind == SourceBlock::Kind::Class || block.kind == SourceBlock::Kind::Struct)) {
            scope.typeIndex = current;
        }
        current = block.parent;
    }
    return scope;
}

auto MissingApiDocumentationRule::isConstructor(
    const SourceIndex &source,
    const el::String &declarationText,
    const SourceBlock &type,
    const el::LineIndex lineIndex) -> bool {
    auto constructorText = declarationText;
    for (const auto &prefix : {"explicit "_el, "constexpr "_el, "inline "_el, "virtual "_el}) {
        if (constructorText.startsWith(prefix)) {
            constructorText =
                constructorText.slice(el::StringSide::Back, el::ByteIndex::end(prefix.length())).trimmed();
        }
    }
    auto constructor = constructorText.startsWith(el::join(type.name, "("_el)) ||
        constructorText.startsWith(el::join("~"_el, type.name, "("_el));
    if (constructor) {
        auto previous = el::String{};
        if (!lineIndex.isZero()) {
            const auto previousLine = lineIndex.decremented();
            previous = source.slice(el::ByteRange{source.lineStart(previousLine), source.lineEnd(previousLine)}, true)
                           .trimmed();
        }
        if (!previous.isEmpty() && !previous.endsWith(";"_el) && !previous.endsWith("{"_el) &&
            !previous.endsWith("}"_el) && previous != "public:"_el && previous != "protected:"_el &&
            previous != "private:"_el && !impl::declarationPreamble(previous)) {
            constructor = false;
        }
        for (
            auto previousLine = lineIndex;
            constructor && !previousLine.isZero() && previousLine.absoluteDistanceTo(lineIndex) < el::LineCount{12};) {
            --previousLine;
            const auto raw = source.line(previousLine).trimmed();
            if (raw.contains("// defaults"_el)) {
                constructor = false;
                break;
            }
            const auto code =
                source.slice(el::ByteRange{source.lineStart(previousLine), source.lineEnd(previousLine)}, true)
                    .trimmed();
            if (code.startsWith("public:"_el) || code.startsWith("protected:"_el) || code.startsWith("private:"_el)) {
                break;
            }
        }
    }
    return constructor;
}

auto MissingApiDocumentationRule::isExempt(
    const SourceIndex &source, const el::ByteIndex start, const el::String &statement) const -> bool {
    auto exempt = false;
    for (const auto &text : _declarationExemptionTexts) {
        exempt |= statement.contains(text);
    }
    if (exempt || statement.contains("override"_el) || statement.contains("= default"_el) ||
        statement.contains("= delete"_el) || impl::standardExtension(source, start, statement)) {
        return true;
    }
    return false;
}

void MissingApiDocumentationRule::scan(const SourceIndex &source, el::List<el::ByteRange> &candidates) const {
    if (isFileExempt(source)) {
        return;
    }
    static const auto cDeclaration = el::RegEx::compile(
        R"(^(?:(?:constexpr|consteval|constinit|explicit|inline|static|virtual)\s+)*(?:class|struct|enum\s+class|using\s+[A-Za-z_]\w*\s*=|auto|void)\b)"_el);
    for (auto lineIndex = el::LineIndex::zero(); lineIndex.isWithin(source.lineCount()); ++lineIndex) {
        const auto lineStart = source.lineStart(lineIndex);
        const auto line = source.slice(el::ByteRange{lineStart, source.lineEnd(lineIndex)}, true);
        const auto compact = line.trimmed();
        if (compact.isEmpty() || compact.startsWith("//"_el)) {
            continue;
        }
        const auto start = lineStart + line.find(compact).distanceFromZero();
        const auto scope = findApiScope(source, start);
        if (!scope || impl::hasApiDocumentation(source, start) || impl::outOfClassMember(source, start)) {
            continue;
        }
        const auto end = source.statementEnd(start);
        const auto statement = source.slice(el::ByteRange{start, end}, true);
        auto declarationText = compact;
        while (declarationText.startsWith("[["_el)) {
            const auto closing = declarationText.find("]]"_el);
            if (closing.isNoIndex()) {
                break;
            }
            declarationText = declarationText.slice(el::StringSide::Back, closing + el::ByteCount{2}).trimmed();
        }
        const auto declaration = cDeclaration->findFirst(declarationText);
        const auto constructor = !declaration && scope->typeIndex.isValid() &&
            isConstructor(source, declarationText, source.blocks().getRef(scope->typeIndex), lineIndex);
        if (!declaration && !constructor) {
            continue;
        }
        if ((compact.startsWith("class "_el) || compact.startsWith("struct "_el) ||
                compact.startsWith("enum class "_el)) &&
            !statement.contains("{"_el)) {
            continue;
        }
        if (declaration && declaration->content().endsWith("void"_el) && !statement.contains("("_el)) {
            continue;
        }
        if (declaration && declaration->content().contains("auto"_el)) {
            const auto opening = statement.find("("_el);
            const auto assignment = statement.find("="_el);
            const auto brace = statement.find("{"_el);
            if (opening.isNoIndex() || (!assignment.isNoIndex() && opening > assignment) ||
                (!brace.isNoIndex() && opening > brace)) {
                continue;
            }
        }
        if (isExempt(source, start, statement)) {
            continue;
        }
        candidates.append(el::ByteRange{start, end});
    }
}

}
