// Copyright (c) 2024-2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "LexerValueTestHelper.hpp"

#include <erbsland/mem/ByteBlock.hpp>

using namespace el::text::literals;

TESTED_TARGETS(Lexer)
TAGS(el::mem::ByteBlock)
class LexerStandardBytesTest final : public UNITTEST_SUBCLASS(LexerValueTestHelper) {
public:
    void testValidSingleLineBytes() {
        // various empty byte sequences.
        WITH_CONTEXT(verifyValidByteData("<>"_el, el::mem::ByteBlock{}));
        WITH_CONTEXT(verifyValidByteData("< >"_el, el::mem::ByteBlock{}));
        WITH_CONTEXT(verifyValidByteData("<      >"_el, el::mem::ByteBlock{}));
        WITH_CONTEXT(verifyValidByteData("<  \t \t \t\t    >"_el, el::mem::ByteBlock{}));

        // test zero bytes, that need to be handled correctly (e.g. catching end byte errors.).
        WITH_CONTEXT(verifyValidByteData("<00>"_el, bytesFromHex("00"_el)));
        WITH_CONTEXT(verifyValidByteData("<0000>"_el, bytesFromHex("0000"_el)));
        WITH_CONTEXT(verifyValidByteData("<00000000>"_el, bytesFromHex("00000000"_el)));
        WITH_CONTEXT(verifyValidByteData("<000000ee>"_el, bytesFromHex("000000ee"_el)));
        WITH_CONTEXT(verifyValidByteData("<000000ee>"_el, bytesFromHex("000000ee"_el)));

        // test all possible digits.
        WITH_CONTEXT(verifyValidByteData(
            "<00112233445566778899aabbccddeeffAABBCCDDEEFF>"_el,
            bytesFromHex("00112233445566778899aabbccddeeffaabbccddeeff"_el)));

        // test valid spacing between the bytes.
        WITH_CONTEXT(verifyValidByteData("<    ab12cd34>"_el, bytesFromHex("ab12cd34"_el)));
        WITH_CONTEXT(verifyValidByteData("<ab     12cd34>"_el, bytesFromHex("ab12cd34"_el)));
        WITH_CONTEXT(verifyValidByteData("<ab 12\tcd 34>"_el, bytesFromHex("ab12cd34"_el)));
        WITH_CONTEXT(verifyValidByteData("<ab12 cd34    >"_el, bytesFromHex("ab12cd34"_el)));
        WITH_CONTEXT(verifyValidByteData("<\tab12\tcd34\t>"_el, bytesFromHex("ab12cd34"_el)));

        // test hex prefix.
        WITH_CONTEXT(verifyValidByteData("<hex:>"_el, el::mem::ByteBlock{}));
        WITH_CONTEXT(verifyValidByteData("<hex: >"_el, el::mem::ByteBlock{}));
        WITH_CONTEXT(verifyValidByteData("<hex:ffee>"_el, bytesFromHex("ffee"_el)));
        WITH_CONTEXT(verifyValidByteData("<hex: ffee>"_el, bytesFromHex("ffee"_el)));
        WITH_CONTEXT(verifyValidByteData("<hex: ff    ee   >"_el, bytesFromHex("ffee"_el)));
    }

    void testInvalidBytes() {
        // unexpected end (with comment after value, it is a syntax error).
        WITH_CONTEXT(verifyErrorInValue("<"_el, {ConfErrorCategory::UnexpectedEnd, ConfErrorCategory::Syntax}));
        WITH_CONTEXT(verifyErrorInValue("<h"_el, {ConfErrorCategory::UnexpectedEnd, ConfErrorCategory::Syntax}));
        WITH_CONTEXT(verifyErrorInValue("<he"_el, {ConfErrorCategory::UnexpectedEnd, ConfErrorCategory::Syntax}));
        WITH_CONTEXT(verifyErrorInValue("<hex"_el, {ConfErrorCategory::UnexpectedEnd, ConfErrorCategory::Syntax}));
        WITH_CONTEXT(verifyErrorInValue("<hex:"_el, {ConfErrorCategory::UnexpectedEnd, ConfErrorCategory::Syntax}));
        WITH_CONTEXT(verifyErrorInValue("<0"_el, {ConfErrorCategory::UnexpectedEnd, ConfErrorCategory::Syntax}));
        WITH_CONTEXT(verifyErrorInValue("<00"_el, {ConfErrorCategory::UnexpectedEnd, ConfErrorCategory::Syntax}));
        WITH_CONTEXT(verifyErrorInValue("<    0"_el, {ConfErrorCategory::UnexpectedEnd, ConfErrorCategory::Syntax}));
        WITH_CONTEXT(verifyErrorInValue("<    00"_el, {ConfErrorCategory::UnexpectedEnd, ConfErrorCategory::Syntax}));

        // odd number of hex digits.
        WITH_CONTEXT(verifyErrorInValue("<0>"_el, ConfErrorCategory::Syntax));
        WITH_CONTEXT(verifyErrorInValue("< 0>"_el, ConfErrorCategory::Syntax));
        WITH_CONTEXT(verifyErrorInValue("< 12 34 5>"_el, ConfErrorCategory::Syntax));
        WITH_CONTEXT(verifyErrorInValue("<123 456>"_el, ConfErrorCategory::Syntax));

        // invalid characters
        WITH_CONTEXT(verifyErrorInValue("<123u56>"_el, ConfErrorCategory::Syntax));
        WITH_CONTEXT(verifyErrorInValue("<123O56>"_el, ConfErrorCategory::Syntax));
        WITH_CONTEXT(verifyErrorInValue("<hex:h23456>"_el, ConfErrorCategory::Syntax));

        // unknown format.
        WITH_CONTEXT(verifyErrorInValue("<base64:23456>"_el, ConfErrorCategory::Unsupported));
    }
};
