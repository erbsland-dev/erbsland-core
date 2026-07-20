// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Name.hpp"

#include "Text.hpp"

#include "../char/NamedChars.hpp"
#include "../utilities/YieldMacros.hpp"

using namespace erbsland::text::literals;

namespace erbsland::conf::impl::lexer {

auto expectRegularOrMetaName(Decoder &decoder, const AcceptedNameEnd acceptedNameEnd) -> NameResult {
    bool isMetaName = false;

    auto nameTransaction = Transaction{decoder};
    if (decoder.character() == nc::at) {
        decoder.next();
        isMetaName = true;
        decoder.expect(CharClass::Letter, "Unexpected character in meta name after at-character."_el);
    }
    // Allow reading one more than the maximum numbers of allowed characters to detect names that are too long.
    while (std::cmp_less_equal(nameTransaction.capturedSize(), limits::maxNameLength)) {
        if (decoder.character() == nc::space) {
            // Use a transaction, as we don't know if the name continues after the space.
            auto spaceTransaction = Transaction{decoder};
            decoder.next();
            if (decoder.character() != CharClass::LetterOrDigit) {
                break;                 // In case something else follows the space, this must be the end of the name.
            }
            spaceTransaction.commit(); // Ok, the name continues.
        } else if (decoder.character() == nc::tab) {
            break;                     // Other than a space, a tab is for sure the end of the name.
        } else if (decoder.character() == nc::underscore) {
            decoder.next();
            if (std::cmp_greater(nameTransaction.capturedSize(), limits::maxNameLength)) {
                break; // stop if the underscore makes the name too long.
            }
            if (decoder.character() != CharClass::LetterOrDigit) {
                if (decoder.character() == CharClass::LineBreakOrEnd ||
                    decoder.character() == CharClass::NameValueSeparator || decoder.character() == CharClass::Spacing) {
                    decoder.throwSyntaxError("A name must not end with an underscore."_el);
                }
                if (decoder.character() == nc::underscore) {
                    decoder.throwSyntaxError("A name must not contain two or more subsequent word separators."_el);
                }
                decoder.throwSyntaxError("Unexpected character in this name."_el);
            }
        } else {
            if (acceptedNameEnd == AcceptedNameEnd::NamePath) {
                // Accept all names ends in a name path `name ...`
                if (decoder.character().isEndOfData() || decoder.character() == nc::namePathSeparator ||
                    decoder.character() == nc::openingSquareBracket) {
                    break;
                }
            }
            if (acceptedNameEnd == AcceptedNameEnd::Section) {
                // Accept all names ends inside a section `[...]`
                if (decoder.character() == CharClass::NameValueSeparator ||
                    decoder.character() == nc::namePathSeparator || decoder.character() == nc::closingSquareBracket) {
                    break;
                }
            }
        }
        decoder.expect(CharClass::LetterOrDigit, "Unexpected character following a regular name."_el);
        while (decoder.character() == CharClass::LetterOrDigit) {
            decoder.next();
            if (std::cmp_greater(nameTransaction.capturedSize(), limits::maxNameLength)) {
                break;
            }
        }
    }
    if (std::cmp_greater(nameTransaction.capturedSize(), limits::maxNameLength)) {
        decoder.throwError(ConfErrorCategory::LimitExceeded, "A name must not exceed 100 characters."_el);
    }
    // Convert the captured name into its normalized form.
    auto name = nameTransaction.capturedString().transformed(text::Char::toIdentifierNormalized);
    nameTransaction.commit();
    return {isMetaName, std::move(name)};
}

auto expectRegularOrMetaNameToken(TokenDecoder &decoder) -> LexerToken {
    auto [isMetaName, name] = expectRegularOrMetaName(decoder, AcceptedNameEnd::Section);
    if (isMetaName) {
        return decoder.createToken(TokenType::MetaName, std::move(name));
    }
    return decoder.createToken(TokenType::RegularName, std::move(name));
}

auto expectTextName(TokenDecoder &decoder) -> LexerToken {
    assert(decoder.character() == nc::doubleQuote);
    text::StringEditor name;
    decoder.next();
    parseText(decoder, name);
    if (name.isEmpty()) {
        if (decoder.character() == nc::doubleQuote) {
            decoder.throwSyntaxError("A text name must not be a multi-line text."_el);
        }
        decoder.throwSyntaxError("A text name must not be empty."_el);
    }
    return decoder.createToken(TokenType::TextName, std::move(name));
}

}
