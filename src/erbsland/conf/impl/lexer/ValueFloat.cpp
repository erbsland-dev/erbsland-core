// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ValueFloat.hpp"

#include "../char/NamedChars.hpp"

#include "../../../err/OverflowError.hpp"
#include "../../../err/ParseError.hpp"

using namespace erbsland::text::literals;

namespace erbsland::conf::impl::lexer {

/// The result when parsing a decimal number.
struct ParseDecimalDigitsResult {
    std::size_t digitCount; ///< The number of digits.
    bool zeroPrefixed;      ///< If the number consists of more than one digit and has a zero prefix.
};

/// Converts a string representation of a floating point number to a Float, performing necessary
/// normalization and error checking. Handles removal of digit separators and leading '+'.
/// Throws if the value is invalid or out of range.
/// @param decoder  Reference to the decoder for error reporting.
/// @param value    The input string containing the floating point value.
/// @return         Parsed Float value.
auto checkAndConvertFloat(TokenDecoder &decoder, const text::String &value) -> Float {
    static const auto digitSeparatorCharacters = text::CharSet{"'"_el};
    auto rawValue = value;
    // Remove a leading plus, if present.
    if (rawValue.startsWith("+"_el)) {
        rawValue = rawValue.slice(text::StringSide::Back, unit::ByteIndex{1U});
    }
    // Remove all digit separators (single quotes).
    rawValue = rawValue.removedAll(digitSeparatorCharacters);
    try {
        return rawValue.toFloatOrThrow<double>();
    } catch (const err::OverflowError &) {
        decoder.throwSyntaxError("The floating point number is out of range."_el);
    } catch (const err::ParseError &) {
        decoder.throwSyntaxError("The floating point number is invalid."_el);
    }
}

/// Parse decimal digits with number separators and return the number of digits, and if the number is zero-prefixed.
[[nodiscard]] inline auto parseDecimalDigits(TokenDecoder &decoder) -> ParseDecimalDigitsResult {
    std::size_t digitCount = 0;
    bool hasZeroPrefix = false;
    while (!decoder.character().isEndOfData()) {
        if (digitCount == 0 && decoder.character() == nc::digit0) {
            hasZeroPrefix = true;
        }
        if (decoder.character() == nc::digitSeparator) {
            if (digitCount == 0) {
                decoder.throwSyntaxError("Number cannot start with a digit separator."_el);
            }
            decoder.next(); // Skip it, but expect another digit.
            if (decoder.character() == nc::digitSeparator) {
                decoder.throwSyntaxError("Number cannot contain two consecutive digit separators."_el);
            }
            if (decoder.character() != CharClass::DecimalDigit) {
                decoder.throwSyntaxOrUnexpectedEndError("Expected another digit after the digit separator."_el);
            }
        }
        if (decoder.character() != CharClass::DecimalDigit) {
            break;
        }
        ++digitCount;
        decoder.next();
    }
    if (hasZeroPrefix && digitCount == 1) {
        hasZeroPrefix = false; // if the number only consists of a single zero digit, this is no prefix.
    }
    return {.digitCount = digitCount, .zeroPrefixed = hasZeroPrefix};
}

auto scanNaN(TokenDecoder &decoder, Transaction &transaction) -> std::optional<LexerToken> {
    decoder.next();
    if (decoder.character() == CharClass::LetterA) {
        decoder.next();
        if (decoder.character() == CharClass::LetterN) {
            decoder.next();
            // Ensure no invalid trailing characters.
            if (decoder.character() != CharClass::ValidAfterValue) {
                decoder.throwSyntaxError("Unexpected characters after 'NaN' literal."_el);
            }
            transaction.commit();
            return decoder.createToken(TokenType::Float, std::numeric_limits<Float>::quiet_NaN());
        }
    }
    return std::nullopt;
}

auto scanInf(TokenDecoder &decoder, Transaction &transaction, const bool isNegative) -> std::optional<LexerToken> {
    if (decoder.character() == CharClass::LetterI) {
        decoder.next();
        if (decoder.character() == CharClass::LetterN) {
            decoder.next();
            if (decoder.character() == CharClass::LetterF) {
                decoder.next();
                if (decoder.character() != CharClass::ValidAfterValue) {
                    decoder.throwSyntaxError("Unexpected characters after “inf” literal."_el);
                }
                transaction.commit();
                if (isNegative) {
                    return decoder.createToken(TokenType::Float, -std::numeric_limits<Float>::infinity());
                }
                return decoder.createToken(TokenType::Float, std::numeric_limits<Float>::infinity());
            }
        }
    }
    return std::nullopt;
}

auto scanLiteralFloat(TokenDecoder &decoder) -> std::optional<LexerToken> {
    // Early exit if there's clearly not a literal float keyword ahead.
    if (decoder.character() != CharClass::FloatLiteralStart) {
        return std::nullopt;
    }
    auto transaction = Transaction{decoder};
    bool isNegative = false;
    if (decoder.character() == CharClass::PlusOrMinus) { // Consume optional sign.
        if (decoder.character() == nc::minus) {
            isNegative = true;
        }
        decoder.next();
    }
    if (decoder.character() == CharClass::LetterN) {
        if (auto token = scanNaN(decoder, transaction)) {
            return token;
        }
    }
    return scanInf(decoder, transaction, isNegative);
}

/// Scans and parses the exponent portion (e.g., 'E+10') of a floating point number.
/// Ensures that an exponent is present and valid, and converts the entire captured float string.
/// @param decoder         The token decoder for reading input and reporting errors.
/// @param transaction     Active transaction holding the string to parse.
/// @return                Parsed floating point LexerToken, or nullopt on failure.
auto scanFloatAfterExponent(TokenDecoder &decoder, Transaction &transaction) -> std::optional<LexerToken> {

    // Allow optional sign after 'E' or 'e'.
    if (decoder.character() == CharClass::PlusOrMinus) {
        decoder.next();
    }
    // Exponent must have at least one digit.
    if (decoder.character() != CharClass::DecimalDigit) {
        decoder.throwSyntaxOrUnexpectedEndError("Expected a decimal digit after the exponent."_el);
    }
    // Read up to 6 decimal digits for the exponent.
    std::size_t digitCount = 0;
    while (decoder.character() == CharClass::DecimalDigit) {
        if (digitCount >= 6) {
            decoder.throwError(ConfErrorCategory::LimitExceeded, "Exponent too long: maximum 6 digits allowed."_el);
        }
        decoder.next();
        digitCount += 1;
    }
    // Require that the exponent is properly terminated.
    if (decoder.character() != CharClass::ValidAfterValue) {
        decoder.throwSyntaxError("Unexpected trailing characters after exponent."_el);
    }
    auto value = checkAndConvertFloat(decoder, transaction.capturedString());
    transaction.commit();
    return decoder.createToken(TokenType::Float, value);
}

/// Scans the portion after the decimal point of a floating point value (including optional exponent).
/// Validates correct delimiter placement and digit count.
/// @param decoder         The token decoder to read input.
/// @param transaction     Reference for transaction capturing the float string.
/// @param totalDigits     The number of digits already encountered before the decimal.
/// @return                Parsed floating point LexerToken or nullopt on parse error.
auto scanFloatAfterDecimalPoint(TokenDecoder &decoder, Transaction &transaction, std::size_t totalDigits)
    -> std::optional<LexerToken> {

    if (decoder.character() == CharClass::DecimalDigit) {
        // Parse digits after decimal point (the fraction part).
        const auto fractionResult = parseDecimalDigits(decoder);
        totalDigits += fractionResult.digitCount;
    } else if (totalDigits == 0) {
        // No digits before or after decimal point is not a valid float.
        if (decoder.character() != CharClass::ValidAfterValue) {
            decoder.throwSyntaxError("Unexpected character after decimal point."_el);
        }
        decoder.throwError(
            ConfErrorCategory::Syntax,
            "Floating-point literal must include digits before or after the decimal point."_el);
    }
    if (totalDigits > 20) {
        decoder.throwError(
            ConfErrorCategory::LimitExceeded,
            "Literal too long: maximum 20 digits allowed (excluding sign and decimal)."_el);
    }
    if (decoder.character() == CharClass::ExponentStart) {
        // Handle scientific notation (e.g., 'e10').
        decoder.next();
        auto token = scanFloatAfterExponent(decoder, transaction);
        if (!token) {
            decoder.throwSyntaxOrUnexpectedEndError("Missing exponent digits: at least one digit required."_el);
        }
        return token;
    }
    // Ensure that the float is properly terminated.
    if (decoder.character() != CharClass::ValidAfterValue) {
        decoder.throwSyntaxError("Unexpected trailing characters after exponent."_el);
    }
    auto value = checkAndConvertFloat(decoder, transaction.capturedString());
    transaction.commit();
    return decoder.createToken(TokenType::Float, value);
}

auto scanFloatFractionOnly(TokenDecoder &decoder) -> std::optional<LexerToken> {
    // Check if the next character could possibly start a float fraction.
    if (!(decoder.character() == CharClass::PlusOrMinus || decoder.character() == nc::decimalPoint)) {
        return std::nullopt;
    }
    auto transaction = Transaction{decoder};
    if (decoder.character() == CharClass::PlusOrMinus) {
        decoder.next();
    }
    if (decoder.character() != nc::decimalPoint) {
        return std::nullopt;
    }
    decoder.next();
    return scanFloatAfterDecimalPoint(decoder, transaction, 0);
}

auto scanFloatWithWholePart(TokenDecoder &decoder) -> std::optional<LexerToken> {
    // Only parse if number start is detected (digit or sign).
    if (decoder.character() != CharClass::NumberStart) {
        return std::nullopt;
    }
    auto transaction = Transaction{decoder};
    if (decoder.character() == CharClass::PlusOrMinus) {
        decoder.next();
    }
    if (decoder.character() != CharClass::DecimalDigit) {
        return std::nullopt;
    }
    // Parse the whole-number part of the float.
    std::size_t totalDigits = 0;
    const auto wholeResult = parseDecimalDigits(decoder);
    totalDigits += wholeResult.digitCount;
    if (decoder.character() == CharClass::ExponentStart) {
        decoder.next();
        // Exponent found, enforce syntax and digit count rules.
        if (wholeResult.zeroPrefixed) {
            decoder.throwSyntaxError("Leading zeros not allowed in floating-point literals."_el);
        }
        if (totalDigits > 20) {
            decoder.throwError(
                ConfErrorCategory::LimitExceeded,
                "Literal too long: maximum 20 digits allowed (excluding sign and decimal)."_el);
        }
        return scanFloatAfterExponent(decoder, transaction);
    }
    // If no decimal point follows, it's not a float value.
    if (decoder.character() != nc::decimalPoint) {
        return std::nullopt;
    }
    if (totalDigits > 1 && wholeResult.zeroPrefixed) {
        decoder.throwSyntaxError("Leading zeros not allowed in floating-point literals."_el);
    }
    decoder.next();
    return scanFloatAfterDecimalPoint(decoder, transaction, totalDigits);
}

}
