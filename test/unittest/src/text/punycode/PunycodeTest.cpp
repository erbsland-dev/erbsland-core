// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/err/ParseError.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/punycode/PunycodeDecoder.hpp>
#include <erbsland/text/punycode/PunycodeEncoder.hpp>
#include <erbsland/unittest/TextHelper.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <string>
#include <vector>

using namespace el::text;
using namespace el::text::literals;
using namespace el::text::punycode;

TESTED_TARGETS(PunycodeEncoder PunycodeDecoder)
class PunycodeTest final : public el::UnitTest {
public:
    void testRfc3492Examples() {
        const auto vectors = std::vector<std::pair<String, String>>{
            {"ليهمابتكلموشعربي؟"_el, "egbpdaj6bu4bxfgehfvwxn"_el},
            {"他们为什么不说中文"_el, "ihqwcrb4cv8a8dqg056pqjye"_el},
            {"他們爲什麽不說中文"_el, "ihqwctvzc91f659drss3x8bo0yb"_el},
            {"Pročprostěnemluvíčesky"_el, "Proprostnemluvesky-uyb24dma41a"_el},
            {"למההםפשוטלאמדבריםעברית"_el, "4dbcagdahymbxekheh6e0a7fei0b"_el},
            {"यहलोगहिन्दीक्योंनहींबोलसकतेहैं"_el, "i1baa7eci9glrd9b2ae1bj0hfcgg6iyaf8o0a1dig0cd"_el},
            {"なぜみんな日本語を話してくれないのか"_el, "n8jok5ay5dzabd5bym9f0cm5685rrjetr6pdxa"_el},
            {"세계의모든사람들이한국어를이해한다면얼마나좋을까"_el,
                "989aomsvi5e83db1d2a355cv1e0vak1dwrv93d5xbh15a0dt30a5jpsd879ccm6fea98c"_el},
            {"почемужеонинеговорятпорусски"_el, "b1abfaaepdrnnbgefbadotcwatmq2g4l"_el},
            {"PorquénopuedensimplementehablarenEspañol"_el, "PorqunopuedensimplementehablarenEspaol-fmd56a"_el},
            {"TạisaohọkhôngthểchỉnóitiếngViệt"_el, "TisaohkhngthchnitingVit-kjcr8268qyxafd2f1b9g"_el},
            {"3年B組金八先生"_el, "3B-ww4c5e180e575a65lsy2b"_el},
            {"安室奈美恵-with-SUPER-MONKEYS"_el, "-with-SUPER-MONKEYS-pc58ag80a8qai00g7n9n"_el},
            {"Hello-Another-Way-それぞれの場所"_el, "Hello-Another-Way--fc4qua05auwb3674vfr0b"_el},
            {"ひとつ屋根の下2"_el, "2-u9tlzr9756bt3uc0v"_el},
            {"MajiでKoiする5秒前"_el, "MajiKoi5-783gue6qz075azm5e"_el},
            {"パフィーdeルンバ"_el, "de-jg4avhby1noc0d"_el},
            {"そのスピードで"_el, "d9juau41awczczp"_el},
        };
        for (const auto &[unicode, encoded] : vectors) {
            runWithContext(
                SOURCE_LOCATION(),
                [&]() -> void {
                    REQUIRE_EQUAL(PunycodeEncoder{unicode}.encodeOrThrow(), encoded);
                    REQUIRE_EQUAL(PunycodeDecoder{encoded}.decodeOrThrow(), unicode);
                },
                [&]() -> std::string { return "RFC 3492 vector failed"; });
        }
    }

    void testPureAsciiDelimiter() {
        REQUIRE_EQUAL(PunycodeEncoder{"abc"_el}.encodeOrThrow(), "abc-"_el);
        REQUIRE_EQUAL(PunycodeDecoder{"abc-"_el}.decodeOrThrow(), "abc"_el);
        REQUIRE_EQUAL(PunycodeEncoder{""_el}.encodeOrThrow(), ""_el);
        REQUIRE_EQUAL(PunycodeDecoder{""_el}.decodeOrThrow(), ""_el);
    }

    void testFailureReportingParity() {
        const auto decoder = PunycodeDecoder{"abc!"_el};
        REQUIRE_FALSE(decoder.decode().has_value());
        REQUIRE_THROWS_AS(el::err::ParseError, decoder.decodeOrThrow());
        const auto invalidBytes = el::unittest::th::stdStringFromHex("C3");
        const auto invalid = String{std::string_view{invalidBytes}};
        const auto encoder = PunycodeEncoder{invalid};
        REQUIRE_FALSE(encoder.encode().has_value());
        REQUIRE_THROWS_AS(el::err::ParseError, encoder.encodeOrThrow());
    }
};
