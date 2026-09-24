// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Parser.hpp"

#include "../constants/Defaults.hpp"
#include "../constants/Limits.hpp"

#include "../../../text/StringEditor.hpp"
#include "../../../text/StringFormat.hpp"

#include <ranges>

namespace erbsland::conf::impl {

using namespace text::literals;

Parser::Parser(SourcePtr documentSource, const ParserSettings &settings) : _settings{settings} {
    // Prepare the stack with the root context.
    _contextStack.reserve(limits::maxDocumentNesting + 1);
    _contextStack.emplace_back(ParserContext::create(0, std::move(documentSource), _settings.placeholderResolver));
}

auto Parser::parse() -> DocumentPtr {
    try {
        Location rootLocation;
        if (!_contextStack.empty()) {
            // Create a location for the document root for better error messages.
            rootLocation = Location{_contextStack.front()->sourceIdentifier()};
        }
        while (hasMoreContent()) {
            initializeCurrentContext();
            if (hasNext()) {
                auto assignment = nextAssignment();
                processAssignment(assignment);
            } else {
                preLeaveProcessing();
                leaveContext();
            }
        }
        auto document = _builder.getDocumentAndReset();
        document->setLocation(rootLocation);
        return document;
    } catch (const ConfError &error) {
        auto enrichedError = error;
        if (error.context().location().has_value() && !_contextStack.empty()) {
            enrichedError = error.withCodeSnippet(_contextStack.back()->codeSnippet(*error.context().location()));
        }
        // close all contexts in case of an error.
        for (const auto &context : std::views::reverse(_contextStack)) {
            try {
                context->close();
            } catch (const ConfError &) {
                // ignore any `ConfError` exceptions while closing the contexts because of an error.
            }
        }
        throw enrichedError;
    }
}

auto Parser::hasMoreContent() const -> bool {
    return !_contextStack.empty();
}

auto Parser::currentContext() -> ParserContext & {
    if (_contextStack.empty()) {
        throw err::LogicError{"Called 'currentContext()` with no context available."};
    }
    return *_contextStack.back();
}

auto Parser::currentContext() const -> const ParserContext & {
    if (_contextStack.empty()) {
        throw err::LogicError{"Called 'currentContext()` with no context available."};
    }
    return *_contextStack.back();
}

void Parser::initializeCurrentContext() {
    if (!currentContext().isInitialized()) {
        // before initializing, verify if we are allowed to access the source.
        if (_settings.accessCheck != nullptr) {
            const AccessSources sources{
                .source = currentContext().sourceIdentifier(),
                .parent = currentContext().parentSourceIdentifier(),
                .root = !_contextStack.empty() ? _contextStack.front()->sourceIdentifier() : nullptr};
            auto location = currentContext().includeLocation();
            if (location.isUndefined()) {
                location = Location{currentContext().sourceIdentifier()};
            }
            try {
                if (_settings.accessCheck->check(sources) != AccessCheckResult::Granted) {
                    throw ConfError{ConfErrorCategory::Access, "Access denied to source."_el, location};
                }
            } catch (const ConfError &error) {
                throw error.withLocation(location);
            }
        }
        // Now as we got access, initialize this context.
        currentContext().initialize();
    }
}

auto Parser::hasNext() const -> bool {
    return currentContext().hasNext();
}

auto Parser::nextAssignment() -> Assignment {
    return currentContext().nextAssignment();
}

void Parser::processAssignment(const Assignment &assignment) {
    switch (assignment.type()) {
    case AssignmentType::EndOfDocument:
        // ignore
        break;
    case AssignmentType::SectionMap:
        _builder.addSectionMap(assignment.namePath(), assignment.location());
        break;
    case AssignmentType::SectionList:
        _builder.addSectionList(assignment.namePath(), assignment.location());
        break;
    case AssignmentType::Value:
        _builder.addValue(assignment.namePath(), assignment.value(), assignment.location());
        break;
    case AssignmentType::MetaValue:
        processMetaValue(assignment);
        break;
    }
}

void Parser::processMetaValue(const Assignment &assignment) {
    if (assignment.namePath().back() == Name::metaSignature()) {
        currentContext().setSignatureText(assignment.value()->asText());
    } else if (assignment.namePath().back() == Name::metaInclude()) {
        const auto includeLevel = currentContext().includeLevel() + 1U;
        if (includeLevel >= limits::maxDocumentNesting) {
            throw ConfError{
                ConfErrorCategory::LimitExceeded,
                text::StringFormat{"The maximum document nesting level of {} is exceeded."_el}.build(
                    limits::maxDocumentNesting),
                assignment.location()};
        }
        SourceResolverContext const resolveContext{
            .includeText = assignment.value()->asText(), .sourceIdentifier = sourceIdentifier()};
        if (_settings.sourceResolver == nullptr) {
            throw ConfError{
                ConfErrorCategory::Unsupported, "The @include meta-command is disabled."_el, assignment.location()};
        }
        SourceListPtr sourceList;
        try {
            sourceList = _settings.sourceResolver->resolve(resolveContext);
        } catch (const ConfError &error) {
            throw error.withLocation(assignment.location());
        }
        if (sourceList == nullptr) {
            throw ConfError{
                ConfErrorCategory::Syntax,
                "The @include meta-command could not be resolved."_el,
                assignment.location()};
        }
        const auto parentSourceIdentifier = sourceIdentifier();
        for (const auto &source : std::views::reverse(*sourceList)) {
            addSourceContext(includeLevel, source, parentSourceIdentifier, assignment.location());
        }
    }
}

void Parser::addSourceContext(
    const std::size_t includeLevel,
    const SourcePtr &source,
    const SourceIdentifierPtr &parentSourceIdentifier,
    const Location &location) {
    if (source == nullptr) {
        throw ConfError{ConfErrorCategory::Syntax, "The source resolver returned a null source."_el, location};
    }
    for (const auto &context : _contextStack) {
        if (*context->sourceIdentifier() == *source->identifier()) {
            throw ConfError{
                ConfErrorCategory::Syntax,
                "An included document is in the list of parent documents (loop detected)."_el,
                location};
        }
    }
    auto newContext = ParserContext::create(includeLevel, source, _settings.placeholderResolver);
    newContext->setIncludeLocation(location);
    newContext->setParentSourceIdentifier(parentSourceIdentifier);
    _contextStack.emplace_back(std::move(newContext));
}

auto Parser::sourceIdentifier() const -> SourceIdentifierPtr {
    return currentContext().sourceIdentifier();
}

auto Parser::signatureText() const -> text::String {
    return currentContext().signatureText();
}

auto Parser::digestText() const -> text::String {
    return text::String::fromJoined({
        defaults::documentHashAlgorithm.toString(),
        " "_el,
        text::String::fromByteBlock(currentContext().digest(), text::ByteFormat::compact()),
    });
}

void Parser::preLeaveProcessing() {
    // Before leaving the context, verify the signature if one is required.
    if (_settings.signatureValidator != nullptr) {
        const SignatureValidatorData data{
            .sourceIdentifier = sourceIdentifier(), .signatureText = signatureText(), .documentDigest = digestText()};
        const auto signatureVerificationResult = _settings.signatureValidator->validate(data);
        if (signatureVerificationResult != SignatureValidatorResult::Accept) {
            throw ConfError{
                ConfErrorCategory::Signature, "Signature verification failed."_el, Location{sourceIdentifier()}};
        }
    } else {
        // The default behavior is to reject a document with a signature if it can't be verified.
        if (!signatureText().isEmpty()) {
            throw ConfError{
                ConfErrorCategory::Signature, "Signature cannot be verified."_el, Location{sourceIdentifier()}};
        }
    }
}

void Parser::leaveContext() {
    if (_contextStack.empty()) {
        throw err::LogicError{"Called 'leaveContext()` with no context available."};
    }
    currentContext().close();
    _contextStack.pop_back();
}

}
