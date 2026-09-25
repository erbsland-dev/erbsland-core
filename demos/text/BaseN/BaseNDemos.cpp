// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/mem/ByteBlock.hpp>
#include <erbsland/text/base_n/BaseNDecoder.hpp>
#include <erbsland/text/base_n/BaseNEncoder.hpp>
#include <erbsland/text/base_n/BaseNFormat.hpp>
#include <erbsland/text/base_n/BaseNFormatFlag.hpp>
#include <erbsland/text/CharSet.hpp>

#include <span>

namespace demo {

/// Carry a small byte value through a text field and recover exactly the original bytes.
///
/// Base-N encoding changes the representation of a `ByteBlock`, not its contents. The encoder and decoder use the
/// same format so the text can cross a text-only boundary without treating the bytes as Unicode characters.
void roundTripSoundTag() {
    // Treat the sound label as bytes that need to pass through a text field.
    const auto original = el::ByteBlock::fromSpan(std::span<const char>{"kaiku", 5U});
    const auto format = el::base_n::BaseNFormat::base64();
    const auto encoded = el::base_n::BaseNEncoder{original, format}.toString();

    // Decode with the same format and compare the recovered bytes.
    const auto recovered = el::base_n::BaseNDecoder{encoded, format}.toDataOrThrow();
    el::io::printLine("Text field: "_el, encoded);
    el::io::printLine("Bytes recovered: "_el, recovered == original);
}

/// Encode a small binary sound label as portable Base64 text.
///
/// `BaseNEncoder` consumes a `ByteBlock` and a matching `BaseNFormat`. The default format is canonical, padded
/// Base64. Choose a specific format factory when the receiving protocol requires another alphabet or layout.
void encodeSoundTag() {
    // Encode the UTF-8 bytes of a short Finnish sound label.
    const auto bytes = el::ByteBlock::fromSpan(std::span<const char>{"kaiku", 5U});
    const auto encoded = el::base_n::BaseNEncoder{bytes}.toString();
    el::io::printLine("Base64: "_el, encoded);

    // The same encoder can produce UTF-16 and UTF-32 text when that is the destination's string width.
    const auto encoder = el::base_n::BaseNEncoder{bytes};
    el::io::printLine("UTF-16 characters: "_el, encoder.toU16String().characterLength().toSizeT());
    el::io::printLine("UTF-32 characters: "_el, encoder.toU32String().characterLength().toSizeT());
}

/// Decode Base64 with an explicit byte limit and choose how to handle invalid input.
///
/// `toData()` returns an empty optional for malformed input or an exceeded size limit. `toDataOrThrow()` preserves
/// the distinction as a parse or range exception. Both methods check the decoded byte count before producing data.
void decodeSoundTag() {
    // Decode a small value with the largest byte count this field permits.
    const auto decoded = el::base_n::BaseNDecoder{"a2Fpa3U="_el}.toData(el::ByteLength{5U});
    el::io::printLine("Decoded bytes: "_el, decoded->length().toSizeT());

    // A malformed payload and an oversized payload both fail in the optional form.
    const auto invalid = el::base_n::BaseNDecoder{"a2Fpa3U!"_el}.toData();
    const auto tooLarge = el::base_n::BaseNDecoder{"a2Fpa3U="_el}.toData(el::ByteLength{4U});
    el::io::printLine("Malformed accepted: "_el, invalid.has_value());
    el::io::printLine("Oversized accepted: "_el, tooLarge.has_value());
}

/// Choose a predefined Base-N alphabet or supply a distinct 16-, 32-, or 64-character alphabet.
///
/// Each alphabet character represents four, five, or six bits. The encoder and decoder must use the same alphabet:
/// Base64 and Base64url, for example, assign different characters to their last two values.
void chooseAlphabet() {
    const auto bytes = el::ByteBlock{el::Byte{0xfbU}, el::Byte{0xffU}};
    const auto base64 = el::base_n::BaseNFormat::base64();
    const auto url = el::base_n::BaseNFormat::base64Url();
    el::io::printLine("Base64: "_el, el::base_n::BaseNEncoder{bytes, base64}.toString());
    el::io::printLine("Base64url: "_el, el::base_n::BaseNEncoder{bytes, url}.toString());

    // A custom Base16 alphabet changes the text representation of the same bytes.
    const auto custom = el::base_n::BaseNFormat{el::U32String{U"FEDCBA9876543210"_el}};
    el::io::printLine("Custom Base16: "_el, el::base_n::BaseNEncoder{bytes, custom}.toString());
    el::io::printLine("Bits per character: "_el, custom.bitsPerCharacter());
}

/// Configure the padding character separately from the policy for emitting and requiring it.
///
/// A padding character must be outside both the alphabet and the ignored whitespace set. A custom alphabet starts
/// without padding; assign a character before enabling either padding flag.
void choosePadding() {
    const auto bytes = el::ByteBlock::fromSpan(std::span<const char>{"kaiku", 5U});
    auto format = el::base_n::BaseNFormat::base64();
    format.setPadding(el::Char{U'~'});
    const auto encoded = el::base_n::BaseNEncoder{bytes, format}.toString();
    el::io::printLine("Alternate padding: "_el, encoded);
    el::io::printLine("Round trip: "_el, el::base_n::BaseNDecoder{encoded, format}.toDataOrThrow() == bytes);
}

/// Select exactly which characters a decoder may ignore between Base-N digits.
///
/// Standard formats accept ASCII whitespace. Replacing that set can make a protocol-specific separator legal while
/// ordinary spaces become invalid. The alphabet, padding, and whitespace sets must remain disjoint.
void chooseWhitespace() {
    auto format = el::base_n::BaseNFormat::base64();
    format.setWhitespace(el::CharSet{U'~'});
    el::io::printLine("Tilde accepted: "_el, el::base_n::BaseNDecoder{"a2Fp~a3U="_el, format}.toData().has_value());
    el::io::printLine("Space accepted: "_el, el::base_n::BaseNDecoder{"a2Fp a3U="_el, format}.toData().has_value());
}

/// Change the three independent Base-N behavior flags.
///
/// `EmitPadding` changes encoded output, `RequirePadding` changes decoding policy, and `WrapLines` changes output
/// layout. `setFlags()` replaces all flags; `addFlags()` and `clearFlags()` adjust an existing format.
void chooseFlags() {
    const auto bytes = el::ByteBlock::fromSpan(std::span<const char>{"kaiku", 5U});
    auto format = el::base_n::BaseNFormat::base64();
    format.clearFlags(el::base_n::BaseNFormatFlag::EmitPadding | el::base_n::BaseNFormatFlag::RequirePadding);
    const auto unpadded = el::base_n::BaseNEncoder{bytes, format}.toString();
    el::io::printLine("Unpadded: "_el, unpadded);
    el::io::printLine("Unpadded accepted: "_el, el::base_n::BaseNDecoder{unpadded, format}.toData().has_value());
    format.addFlags(el::base_n::BaseNFormatFlag::RequirePadding);
    el::io::printLine("Padding now required: "_el, el::base_n::BaseNDecoder{unpadded, format}.toData().has_value());
}

/// Accept an unpadded Base64 field only when its protocol allows that spelling.
///
/// Clearing `RequirePadding` permits a missing final padding character. Padding that is present must still be
/// canonical, so adding an incorrect number of padding characters does not become valid.
void decodePaddingPolicy() {
    auto format = el::base_n::BaseNFormat::base64();
    el::io::printLine(
        "Default accepts a2Fpa3U: "_el, el::base_n::BaseNDecoder{"a2Fpa3U"_el, format}.toData().has_value());

    format.clearFlags(el::base_n::BaseNFormatFlag::RequirePadding);
    el::io::printLine(
        "Optional padding accepts a2Fpa3U: "_el, el::base_n::BaseNDecoder{"a2Fpa3U"_el, format}.toData().has_value());
    el::io::printLine(
        "Incorrect padding accepted: "_el, el::base_n::BaseNDecoder{"a2Fpa3U=="_el, format}.toData().has_value());
}

/// Wrap encoded output by setting a line length, separator, and `WrapLines` flag.
///
/// The line length counts encoded characters, including padding. Each separator character must also be accepted
/// whitespace during decoding. Wrapping inserts no separator after the final line.
void wrapLines() {
    const auto bytes = el::ByteBlock::fromSpan(std::span<const char>{"kaiku", 5U});
    auto format = el::base_n::BaseNFormat::base64();
    format.setLineLength(el::CpLength{4U})
        .setLineSeparator(el::U32String{U"\n"_el})
        .addFlags(el::base_n::BaseNFormatFlag::WrapLines);
    const auto wrapped = el::base_n::BaseNEncoder{bytes, format}.toString();
    el::io::printLine("Wrapped:\n"_el, wrapped);
    el::io::printLine("Round trip: "_el, el::base_n::BaseNDecoder{wrapped, format}.toDataOrThrow() == bytes);
}

}
