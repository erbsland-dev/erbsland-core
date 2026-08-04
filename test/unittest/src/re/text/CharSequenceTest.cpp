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
        REQUIRE(seq.sequence().empty());
        REQUIRE_EQUAL(seq.hash(), std::size_t{0});
        REQUIRE_EQUAL(seq.begin(), seq.end());
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
            REQUIRE_NOT_EQUAL(it, seq.end());
            REQUIRE_EQUAL(it->toRawValue(), ch.toRawValue());
            ++it;
        }
        REQUIRE_EQUAL(it, seq.end());

        // verify hash accessor and std::hash specialization
        REQUIRE_EQUAL(seq.hash(), expectedHash);
        REQUIRE_EQUAL(std::hash<CharSequence>{}(seq), expectedHash);
    }

    void testComparison() {
        // copied inline sequence
        CharSequence a;
        a.append(Char{U'x'});
        a.append(Char{U'y'});

        CharSequence b = a;
        REQUIRE_EQUAL(a, b);
        REQUIRE_EQUAL(a, b);

        // different instance with identical content
        CharSequence c;
        c.append(Char{U'x'});
        c.append(Char{U'y'});
        REQUIRE_EQUAL(a, c); // same hash, different SequencePtr, vector compare true
        REQUIRE_EQUAL(a, c);

        // different content -> different hash -> early false
        CharSequence d;
        d.append(Char{U'x'});
        d.append(Char{U'z'});
        REQUIRE_NOT_EQUAL(a, d);
        REQUIRE_NOT_EQUAL(a, d);
    }

    void testConditionalDetachOnCopy() {
        CharSequence original;
        for (auto index = std::size_t{}; index < 9U; ++index) {
            original.append(Char{static_cast<char32_t>(U'1' + index)});
        }

        CharSequence copy = original; // shared state
        // Initially both sequences must point to the same buffer
        REQUIRE_EQUAL(original.sequence().data(), copy.sequence().data());
        REQUIRE_EQUAL(original.hash(), copy.hash());

        // Mutate the copy: must detach and not affect the original
        copy.append(Char{U'a'}); // triggers conditionalDetach()
        REQUIRE_NOT_EQUAL(original.sequence().data(), copy.sequence().data());

        // Contents diverge now
        REQUIRE_EQUAL(original.size(), std::size_t{9});
        REQUIRE_EQUAL(copy.size(), std::size_t{10});

        // Hashes diverge accordingly
        REQUIRE_NOT_EQUAL(original.hash(), copy.hash());

        // Ensure original content intact
        std::vector<char32_t> oc;
        for (const auto ch : original) {
            oc.push_back(ch.toRawValue());
        }
        REQUIRE_EQUAL(oc.size(), std::size_t{9});
        REQUIRE_EQUAL(oc[0], U'1');
        REQUIRE_EQUAL(oc[8], U'9');
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
