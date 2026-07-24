// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/re/impl/text/CharSequence.hpp>
#include <erbsland/re/RegExError.hpp>
#include <erbsland/re/StdFormat.hpp>
#include <erbsland/unittest/UnitTest.hpp>
#include <erbsland/util/HashHelper.hpp>

using namespace el::re;
using el::text::Char;
using el::util::combineHash;
using impl::CharSequence;

TESTED_TARGETS(CharSequence)
TAGS(Text CharSequences)
class CharSequenceTest final : public el::UnitTest {
public:
    CharSequence seq;

    void testDefault() {
        seq = {};
        REQUIRE_EQUAL(seq.size(), std::size_t{0});
        REQUIRE(seq.sequence() != nullptr);
        REQUIRE_EQUAL(seq.hash(), std::size_t{0});
        REQUIRE(seq.begin() == seq.end());
    }

    void testAppendAndHash() {
        seq = {};
        // Append a few characters and check size/content/hash
        std::vector<Char> expected{Char{U'a'}, Char{U'\n'}, Char{0x1F600U}}; // 'a', newline, 😀
        std::size_t expectedHash = 0;
        for (const auto &ch : expected) {
            expectedHash = combineHash(expectedHash, std::hash<char32_t>{}(ch.toRawValue()));
            seq.append(ch);
        }

        REQUIRE_EQUAL(seq.size(), expected.size());

        // verify contents by iterating begin/end
        auto it = seq.begin();
        for (const auto &ch : expected) {
            REQUIRE(it != seq.end());
            REQUIRE_EQUAL(it->toRawValue(), ch.toRawValue());
            ++it;
        }
        REQUIRE(it == seq.end());

        // verify hash accessor and std::hash specialization
        REQUIRE_EQUAL(seq.hash(), expectedHash);
        REQUIRE_EQUAL(std::hash<CharSequence>{}(seq), expectedHash);
    }

    void testComparison() {
        // same instance (shared_ptr identity fast-path)
        CharSequence a;
        a.append(Char{U'x'});
        a.append(Char{U'y'});

        CharSequence b = a; // b shares the same underlying vector and hash
        REQUIRE(a == b);
        REQUIRE_FALSE(a != b);

        // different instance with identical content (forces vector compare branch)
        CharSequence c;
        c.append(Char{U'x'});
        c.append(Char{U'y'});
        REQUIRE(a == c); // same hash, different SequencePtr, vector compare true
        REQUIRE_FALSE(a != c);

        // different content -> different hash -> early false
        CharSequence d;
        d.append(Char{U'x'});
        d.append(Char{U'z'});
        REQUIRE_FALSE(a == d);
        REQUIRE(a != d);
    }

    void testConditionalDetachOnCopy() {
        CharSequence original;
        original.append(Char{U'1'});
        original.append(Char{U'2'});

        CharSequence copy = original; // shared state
        // Initially both sequences must point to the same buffer
        REQUIRE(original.sequence() == copy.sequence());
        REQUIRE_EQUAL(original.hash(), copy.hash());

        // Mutate the copy: must detach and not affect the original
        copy.append(Char{U'3'}); // triggers conditionalDetach()
        REQUIRE(original.sequence() != copy.sequence());

        // Contents diverge now
        REQUIRE_EQUAL(original.size(), std::size_t{2});
        REQUIRE_EQUAL(copy.size(), std::size_t{3});

        // Hashes diverge accordingly
        REQUIRE(original.hash() != copy.hash());

        // Ensure original content intact
        std::vector<char32_t> oc;
        for (const auto ch : original) {
            oc.push_back(ch.toRawValue());
        }
        REQUIRE_EQUAL(oc.size(), std::size_t{2});
        REQUIRE_EQUAL(oc[0], U'1');
        REQUIRE_EQUAL(oc[1], U'2');
    }

    void testNullCharacterAndInvalidCharacterBoundary() {
        seq = {};
        seq.append(Char::null());
        REQUIRE_EQUAL(seq.size(), 1U);
        REQUIRE(seq.begin()->isNull());

        REQUIRE_THROWS_AS(RegExError, seq.append(Char::endOfData()));
        REQUIRE_THROWS_AS(RegExError, seq.append(Char{0xD800U}));
        REQUIRE_EQUAL(seq.size(), 1U);
    }
};
