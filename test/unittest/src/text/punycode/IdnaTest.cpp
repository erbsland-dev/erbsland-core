// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/err/ParseError.hpp>
#include <erbsland/text/CharSet.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/punycode/impl/IdnaData.hpp>
#include <erbsland/text/punycode/PunycodeDecoder.hpp>
#include <erbsland/text/punycode/PunycodeEncoder.hpp>
#include <erbsland/text/StringEditor.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <cstddef>

using namespace el::text;
using namespace el::text::literals;
using namespace el::text::punycode;

TESTED_TARGETS(PunycodeEncoder PunycodeDecoder PunycodeOptions)
class IdnaTest final : public el::UnitTest {
    using IdnaBidi = el::text::punycode::impl::IdnaBidi;
    using IdnaJoining = el::text::punycode::impl::IdnaJoining;
    using IdnaRange = el::text::punycode::impl::IdnaRange;
    using IdnaScript = el::text::punycode::impl::IdnaScript;
    using IdnaStatus = el::text::punycode::impl::IdnaStatus;

public:
    void testLabelAndDomainRoundTrips() {
        const auto labelOptions = PunycodeOptions::idna2008Label();
        REQUIRE_EQUAL((PunycodeEncoder{"bücher"_el, labelOptions}.encodeOrThrow()), "xn--bcher-kva"_el);
        REQUIRE_EQUAL((PunycodeDecoder{"XN--BCHER-KVA"_el, labelOptions}.decodeOrThrow()), "bücher"_el);

        const auto options = PunycodeOptions::network();
        REQUIRE_EQUAL(
            (PunycodeEncoder{"www.bücher.example"_el, options}.encodeOrThrow()), "www.xn--bcher-kva.example"_el);
        REQUIRE_EQUAL(
            (PunycodeDecoder{"WWW.XN--BCHER-KVA.EXAMPLE"_el, options}.decodeOrThrow()), "www.bücher.example"_el);
    }

    void testAsciiCaseAndNfc() {
        const auto options = PunycodeOptions::network();
        REQUIRE_EQUAL((PunycodeEncoder{"EXAMPLE.COM"_el, options}.encodeOrThrow()), "example.com"_el);
        REQUIRE_EQUAL((PunycodeEncoder{"bücher.example"_el, options}.encodeOrThrow()), "xn--bcher-kva.example"_el);
    }

    void testStrictRejections() {
        const auto options = PunycodeOptions::network();
        REQUIRE_FALSE((PunycodeEncoder{"BÜCHER.example"_el, options}.encode().has_value()));
        REQUIRE_FALSE((PunycodeEncoder{"😀.example"_el, options}.encode().has_value()));
        REQUIRE_FALSE((PunycodeEncoder{"foo_bar.example"_el, options}.encode().has_value()));
        REQUIRE_FALSE((PunycodeEncoder{"example。com"_el, options}.encode().has_value()));
        REQUIRE_FALSE((PunycodeEncoder{"a..b"_el, options}.encode().has_value()));
        REQUIRE_FALSE((PunycodeEncoder{"a."_el, options}.encode().has_value()));
        REQUIRE_FALSE((PunycodeEncoder{"xn--abc-.example"_el, options}.encode().has_value()));
        REQUIRE_FALSE((PunycodeDecoder{"xn--abc-.example"_el, options}.decode().has_value()));
        REQUIRE_FALSE((PunycodeDecoder{"xn--.example"_el, options}.decode().has_value()));
    }

    void testContextAndBidi() {
        const auto options = PunycodeOptions::idna2008Label();
        REQUIRE((PunycodeEncoder{"l·l"_el, options}.encode().has_value()));
        REQUIRE_FALSE((PunycodeEncoder{"a·b"_el, options}.encode().has_value()));
        REQUIRE((PunycodeEncoder{"نامه‌ای"_el, options}.encode().has_value()));
        REQUIRE_FALSE((PunycodeEncoder{"a‌b"_el, options}.encode().has_value()));
        REQUIRE_FALSE((PunycodeEncoder{"א1١"_el, options}.encode().has_value()));
        REQUIRE((PunycodeEncoder{"123"_el, options}.encode().has_value()));
        REQUIRE_FALSE((PunycodeEncoder{"א.123"_el, PunycodeOptions::network()}.encode().has_value()));

        const auto rtlALabel = PunycodeEncoder{"א"_el, options}.encodeOrThrow();
        auto encodedDomain = StringEditor{rtlALabel};
        encodedDomain.append(".123"_el);
        REQUIRE_FALSE((PunycodeEncoder{encodedDomain, PunycodeOptions::network()}.encode().has_value()));
    }

    void testAllowedCharacterFilter() {
        auto allowed = CharSet{};
        allowed.add(CharRange{U'a', U'z'});
        allowed.add(Char{U'-'});
        const auto options = PunycodeOptions::idna2008Label().setAllowedCharacters(allowed);
        REQUIRE((PunycodeEncoder{"allowed"_el, options}.encode().has_value()));
        REQUIRE_FALSE((PunycodeEncoder{"bücher"_el, options}.encode().has_value()));
    }

    void testDnsLimits() {
        auto label = StringEditor{};
        label.append(Char{U'a'}, el::unit::CpLength{63U});
        REQUIRE((PunycodeEncoder{label, PunycodeOptions::idna2008Label()}.encode().has_value()));
        label.append(Char{U'a'});
        REQUIRE_FALSE((PunycodeEncoder{label, PunycodeOptions::idna2008Label()}.encode().has_value()));
    }

    void testCompactRuntimeData() {
        REQUIRE_EQUAL(sizeof(IdnaRange), std::size_t{6U});

        const auto ascii = el::text::punycode::impl::idnaAttributes(U'a');
        REQUIRE(ascii.status() == IdnaStatus::PValid);
        REQUIRE(ascii.bidi() == IdnaBidi::L);

        REQUIRE(el::text::punycode::impl::idnaAttributes(U'α').script() == IdnaScript::Greek);
        REQUIRE(el::text::punycode::impl::idnaAttributes(U'一').script() == IdnaScript::Han);
        REQUIRE(el::text::punycode::impl::idnaAttributes(U'א').script() == IdnaScript::Hebrew);
        REQUIRE(el::text::punycode::impl::idnaAttributes(U'あ').script() == IdnaScript::Hiragana);
        REQUIRE(el::text::punycode::impl::idnaAttributes(U'ア').script() == IdnaScript::Katakana);
        REQUIRE(el::text::punycode::impl::idnaAttributes(U'\u094D').isVirama());
        REQUIRE(el::text::punycode::impl::idnaAttributes(U'\u200C').status() == IdnaStatus::ContextJ);

        const auto emoji = el::text::punycode::impl::idnaAttributes(U'😀');
        REQUIRE(emoji.status() == IdnaStatus::Disallowed);
        REQUIRE(emoji.bidi() == IdnaBidi::Unknown);
        REQUIRE(emoji.joining() == IdnaJoining::Other);
        REQUIRE(emoji.script() == IdnaScript::None);
        REQUIRE_FALSE(emoji.isVirama());
        REQUIRE(el::text::punycode::impl::idnaAttributes(U'\U000F0000').status() == IdnaStatus::Disallowed);
    }
};
