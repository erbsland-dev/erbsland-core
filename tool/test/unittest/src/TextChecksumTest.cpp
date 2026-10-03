// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ProjectFixture.hpp"

#include <erbsland/Hasher.hpp>
#include <erbsland/StringEncoder.hpp>
#include <tool/frame/TextChecksum.hpp>

TESTED_TARGETS(calculateTextChecksum ProjectFiles)
class TextChecksumTest final : public UNITTEST_SUBCLASS(ProjectFixture) {
public:
    void testLineEndingsAndSignificantBytes() {
        for (const auto algorithm : {el::HashAlgorithm::Sha2_256, el::HashAlgorithm::Sha3_256}) {
            auto hasher = el::Hasher{algorithm};
            hasher.update("a\nb\nc\n"_el);
            const auto expected = hasher.finalize();
            for (const auto &text : el::StringList{"a\nb\nc\n"_el, "a\r\nb\r\nc\r\n"_el, "a\rb\nc\r\n"_el}) {
                REQUIRE_EQUAL(tool::frame::calculateTextChecksum(text, algorithm), expected);
                REQUIRE_EQUAL(
                    tool::frame::calculateTextChecksum(
                        el::StringEncoder{text}.encode(el::StringEncoding::Utf8), algorithm),
                    expected);
            }
            for (
                const auto &text :
                el::StringList{"a\nb\nc"_el, "a \nb\nc\n"_el, "\uFEFFa\nb\nc\n"_el, "a\nB\nc\n"_el}) {
                REQUIRE(tool::frame::calculateTextChecksum(text, algorithm) != expected);
            }
            REQUIRE(
                tool::frame::calculateTextChecksum(""_el, algorithm) !=
                tool::frame::calculateTextChecksum("\r"_el, algorithm));
        }
    }
    void testConfinedFileChecksumPreservesBom() {
        const auto text = "\uFEFFfirst\r\nsecond\r"_el;
        write("input.txt"_el, text);
        REQUIRE_EQUAL(
            files().textChecksum(el::Path{"input.txt"_el}, el::HashAlgorithm::Sha2_256),
            tool::frame::calculateTextChecksum(text, el::HashAlgorithm::Sha2_256));
        REQUIRE_THROWS(files().textChecksum(el::Path{"../outside.txt"_el}, el::HashAlgorithm::Sha2_256));
        REQUIRE_THROWS(files().textChecksum(el::Path{"missing.txt"_el}, el::HashAlgorithm::Sha2_256));
    }
};
