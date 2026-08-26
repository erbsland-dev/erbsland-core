// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "AssignmentStream.hpp"

#include "../char/NamedChars.hpp"

#include "../../../mem/ByteBlockEditor.hpp"
#include "../../../re/RegEx.hpp"
#include "../../../text/StringFormat.hpp"
#include "../../../text/StringList.hpp"

namespace erbsland::conf::impl {

using namespace text::literals;

auto AssignmentStream::handleMultiLineValueList() -> std::vector<ValuePtr> {
    std::vector<ValuePtr> valueList;
    while (token().type() == TokenType::MultiLineValueListSeparator) {
        auto bulletLocation = currentLocation();
        expectNext(); // consume the list separator.
        auto subValueList = handleValueOrValueList();
        ValuePtr value;
        if (subValueList.size() == 1) {
            value = subValueList.front();
        } else {
            value = Value::createValueList(std::move(subValueList));
        }
        value->setLocation(bulletLocation);
        valueList.emplace_back(std::move(value));
        if (token().type() != TokenType::Indentation) {
            // When the next line doesn't start with an indentation, the multi-line list ends here.
            // Empty lines are not allowed in multi-line lists.
            break;
        }
        expectNext(TokenType::MultiLineValueListSeparator); // Consume the indentation and expect the next bullet
    }
    return valueList;
}

auto AssignmentStream::handleMultiLineText() -> text::String {
    expectNext(TokenType::LineBreak, TokenType::MultiLineCodeLanguage); // Consume the open sequence.
    if (token().type() == TokenType::MultiLineCodeLanguage) {
        expectNext(TokenType::LineBreak);                               // Consume the code language token.
    }
    expectNext(TokenType::Indentation, TokenType::LineBreak);
    bool isSecondLine = false;
    text::StringEditor text;
    while (token().type() == TokenType::Indentation || token().type() == TokenType::LineBreak) {
        // If the line is just a line-break, it is an empty line.
        if (token().type() != TokenType::LineBreak) {
            // If it isn't a line-break, it is an indentation.
            expectNext(
                TokenType::MultiLineCode,
                TokenType::MultiLineText,
                TokenType::MultiLineTextClose,
                TokenType::MultiLineCodeClose,
                TokenType::LineBreak);
            if (token().type() == TokenType::MultiLineTextClose || token().type() == TokenType::MultiLineCodeClose) {
                next(); // Consume the close sequence.
                verifyAndConsumeEndOfLine();
                break;
            }
            if (isSecondLine) {
                text.append("\n"_el);
            }
            if (token().type() != TokenType::LineBreak) {
                text.append(std::get<text::String>(token().content())); // append the text
                expectNext(TokenType::LineBreak);
            }
        } else {
            // empty line
            if (isSecondLine) {
                text.append("\n"_el);
            }
        }
        expectNext(TokenType::LineBreak, TokenType::Indentation);
        isSecondLine = true;
    }
    return text;
}

auto AssignmentStream::handleMultiLineRegEx() -> text::String {
    // Consume the open sequence and expect an indentation or an empty line.
    expectNext(TokenType::LineBreak);
    expectNext(TokenType::Indentation, TokenType::LineBreak);
    bool isSecondLine = false;
    text::StringEditor text;
    while (token().type() == TokenType::Indentation || token().type() == TokenType::LineBreak) {
        if (token().type() != TokenType::LineBreak) {
            expectNext(TokenType::MultiLineRegex, TokenType::MultiLineRegexClose, TokenType::LineBreak);
            if (token().type() == TokenType::MultiLineRegexClose) {
                next(); // Consume the close sequence.
                verifyAndConsumeEndOfLine();
                break;
            }
            if (isSecondLine) {
                text.append("\n"_el);
            }
            if (token().type() != TokenType::LineBreak) {
                text.append(std::get<text::String>(token().content())); // append the text
                expectNext(TokenType::LineBreak);
            }
        } else {
            // empty line
            if (isSecondLine) {
                text.append("\n"_el);
            }
        }
        expectNext(TokenType::LineBreak, TokenType::Indentation);
        isSecondLine = true;
    }
    return text;
}

auto AssignmentStream::createRegEx(const text::String &pattern, const bool isVerbose) const -> re::RegExPtr {
    auto flags = re::Flags{};
    if (isVerbose) {
        flags |= re::Flag::Verbose;
    }
    auto settings = re::Settings{};
    // ELCL permits empty expressions and alternatives. Preserve these language semantics while using the Core
    // compiler, whose general-purpose defaults intentionally reject them.
    settings.enableFeature(re::Feature::EmptyGroups);
    settings.enableFeature(re::Feature::EmptyAlternatives);
    return re::RegEx::lazyCompile(pattern, flags, settings);
}

auto AssignmentStream::handleMultiLineBytes() -> mem::ByteBlock {
    // Consume the open sequence and expect, skip the format and expect an empty line or indentation.
    expectNext(TokenType::LineBreak, TokenType::MultiLineBytesFormat);
    if (token().type() == TokenType::MultiLineBytesFormat) {
        expectNext(TokenType::LineBreak); // Consume the format (ignored, as only hex is supported).
    }
    expectNext(TokenType::Indentation, TokenType::LineBreak);
    mem::ByteBlockEditor result;
    while (token().type() == TokenType::Indentation || token().type() == TokenType::LineBreak) {
        if (token().type() != TokenType::LineBreak) {
            expectNext(TokenType::MultiLineBytes, TokenType::MultiLineBytesClose, TokenType::LineBreak);
            if (token().type() == TokenType::MultiLineBytesClose) {
                next(); // Consume the close sequence.
                verifyAndConsumeEndOfLine();
                break;
            }
            if (token().type() != TokenType::LineBreak) {
                result.append(std::get<mem::ByteBlock>(token().content())); // append the text
                expectNext(TokenType::LineBreak);
            }
        }
        expectNext(TokenType::Indentation, TokenType::LineBreak);
    }
    return mem::ByteBlock{result};
}

auto AssignmentStream::handleSection() -> Assignment {
    const bool isSectionList = token().type() == TokenType::SectionListOpen;
    bool isRelativePath = false;
    auto openLocation = currentLocation(); // Store the location where the section definition starts.
    expectNext(TokenType::NamePathSeparator, TokenType::RegularName, TokenType::TextName);
    NamePath namePath;
    if (token().type() == TokenType::NamePathSeparator) {
        // If the name starts with a name seperator, this describes a relative path.
        isRelativePath = true;
        expectNext(TokenType::RegularName, TokenType::TextName);
    }
    while (!(token().type() == TokenType::SectionListClose || token().type() == TokenType::SectionMapClose)) {
        if (namePath.size() >= limits::maxNamePathLength) {
            throwLimitExceededError("A name path must not exceed 10 name components."_el);
        }
        if (token().type() == TokenType::RegularName) {
            namePath.append(Name::createRegular(std::get<text::String>(token().content())));
        } else {
            namePath.append(Name::createText(std::get<text::String>(token().content())));
        }
        expectNext(TokenType::NamePathSeparator, TokenType::SectionListClose, TokenType::SectionMapClose);
        if (token().type() != TokenType::NamePathSeparator) {
            break; // if we didn't get a seperator, the section is closed.
        }
        expectNext(TokenType::RegularName, TokenType::TextName);
    }
    next(); // Consume the section closing sequence.
    verifyAndConsumeEndOfLine();
    // Handle relative paths at the end for better error reporting.
    if (isRelativePath) {
        if (_lastAbsolutePath.empty()) {
            throw ConfError{
                ConfErrorCategory::Syntax,
                "There is no absolute section definition before this relative one."_el,
                openLocation,
                namePath};
        }
        namePath.prepend(_lastAbsolutePath);
    } else {
        _lastAbsolutePath = namePath;
    }
    _currentSectionPath = namePath;
    _documentArea = DocumentArea::AfterSection;
    return Assignment{
        isSectionList ? AssignmentType::SectionList : AssignmentType::SectionMap,
        std::move(namePath),
        std::move(openLocation),
        {} // no value for sections.
    };
}

void AssignmentStream::verifyFeatures(const text::String &featureText) const {
    static const auto supportedFeatures = std::array<text::String, 19>{
        "core"_el,
        "minimum"_el,
        "standard"_el,
        "advanced"_el,
        "all"_el,
        "float"_el,
        "byte-count"_el,
        "multi-line"_el,
        "section-list"_el,
        "value-list"_el,
        "text-names"_el,
        "date-time"_el,
        "code"_el,
        "byte-data"_el,
        "include"_el,
        "regex"_el,
        "time-delta"_el,
        "validation"_el,
        "signature"_el,
    };
    text::StringList requestedFeatures;
    text::StringEditor currentFeature;
    std::size_t currentCharacterIndex = 0;
    auto featureIndex = unit::ByteIndex{};
    while (featureIndex.isWithin(featureText.length())) {
        const auto character = featureText.readCharAndAdvance(featureIndex);
        if (character == CharClass::Spacing) {
            // Add the current read feature and clear the string.
            if (!currentFeature.isEmpty()) {
                requestedFeatures.append(text::String{currentFeature});
                currentFeature.clear();
            }
        } else if (character == CharClass::Letter || character == nc::minus) {
            currentFeature.append(character.toAsciiLowercase());
        } else {
            throw ConfError{
                ConfErrorCategory::Syntax,
                text::StringFormat{"Unsupported character in @features text at index {}."_el}.build(
                    currentCharacterIndex),
                currentLocation(),
                NamePath{Name::createRegular("@features"_el)}};
        }
        ++currentCharacterIndex;
    }
    if (!currentFeature.isEmpty()) {
        requestedFeatures.append(text::String{currentFeature});
    }
    for (const auto &feature : requestedFeatures) {
        if (std::ranges::find(supportedFeatures, feature) == supportedFeatures.end()) {
            throw ConfError{
                ConfErrorCategory::Unsupported,
                text::StringFormat{"This parser does not support the feature '{}'."_el}.build(feature),
                currentLocation()};
        }
    }
    // At this point, all features are successfully verified.
}

}
