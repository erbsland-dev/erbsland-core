// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/Date.hpp>
#include <erbsland/Literals.hpp>
#include <erbsland/log/impl/LogLineFormatter.hpp>
#include <erbsland/LogLevelFormat.hpp>
#include <erbsland/LogMessageTruncation.hpp>
#include <erbsland/LogNameFormat.hpp>
#include <erbsland/Time.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <utility>

using namespace el::text::literals;

TESTED_TARGETS(LogLineFormatter)
class LogLineFormatterTest final : public el::UnitTest {
public:
    void testReusesCompiledPatternAndOwnsSettings() {
        auto settings = el::log::LogLineFormat{};
        settings.setPattern("ä {{ {level} }} {name}: {message}/{message}"_el)
            .setLevelFormat(el::log::LogLevelFormat::FullUpper)
            .setNameFormat(el::log::LogNameFormat::Leaf);
        const auto formatter = el::log::impl::LogLineFormatter{settings};
        settings.setPattern("changed"_el).setNameFormat(el::log::LogNameFormat::Full);

        const auto first = formatter.format(makeEntry("一"_el));
        const auto second = formatter.format(makeEntry("二"_el));
        REQUIRE_EQUAL(first->text(), "ä { WARNING } query: 一/一"_el);
        REQUIRE_EQUAL(second->text(), "ä { WARNING } query: 二/二"_el);
        REQUIRE_EQUAL(first->text(), "ä { WARNING } query: 一/一"_el);
        REQUIRE_EQUAL(first->segments().size(), std::size_t{8U});
        REQUIRE_EQUAL(first->segments()[0].part, el::log::LogLinePart::Literal);
        REQUIRE_EQUAL(first->segments()[1].part, el::log::LogLinePart::Level);
        REQUIRE_EQUAL(first->segments()[3].part, el::log::LogLinePart::Name);
        REQUIRE_EQUAL(first->segments()[5].part, el::log::LogLinePart::Message);
        REQUIRE_EQUAL(first->segments()[7].part, el::log::LogLinePart::Message);
    }

    void testLiteralOnlyAndAdjacentPlaceholders() {
        auto settings = el::log::LogLineFormat{};
        settings.setPattern("{{literal}} 一"_el);
        REQUIRE_EQUAL(
            el::log::impl::LogLineFormatter{settings}.format(makeEntry("unused"_el))->text(), "{literal} 一"_el);
        settings.setPattern("{message}{message}"_el);
        const auto line = el::log::impl::LogLineFormatter{settings}.format(makeEntry("ä"_el));
        REQUIRE_EQUAL(line->text(), "ää"_el);
        REQUIRE_EQUAL(line->segments().size(), std::size_t{2U});
    }

    void testTimestampAndFirstLineTruncation() {
        auto settings = el::log::LogLineFormat{};
        settings.setPattern("{time} {message}"_el)
            .setMessageTruncation(el::log::LogMessageTruncation::FirstLine)
            .setTruncationMark("…"_el);
        const auto formatter = el::log::impl::LogLineFormatter{settings};
        REQUIRE_EQUAL(formatter.format(makeEntry("first\nsecond"_el))->text(), "2026-08-31 12:34:56Z first…"_el);
        REQUIRE_EQUAL(formatter.format(makeEntry("single"_el))->text(), "2026-08-31 12:34:56Z single"_el);
    }

    void testCharacterAndTotalLineTruncationWithRepeatedMessages() {
        auto settings = el::log::LogLineFormat{};
        settings.setPattern("[{message}]/{message}"_el)
            .setMessageTruncation(el::log::LogMessageTruncation::CharacterCount)
            .setMessageLimit(el::unit::CpCount{3U});
        REQUIRE_EQUAL(el::log::impl::LogLineFormatter{settings}.format(makeEntry("äöüß"_el))->text(), "[äö…]/äö…"_el);
        settings.setMessageTruncation(el::log::LogMessageTruncation::TotalLineLength)
            .setMessageLimit(el::unit::CpCount{9U});
        const auto formatter = el::log::impl::LogLineFormatter{settings};
        REQUIRE_EQUAL(formatter.format(makeEntry("äöüß"_el))->text(), "[äö…]/äö…"_el);
        REQUIRE_EQUAL(formatter.format(makeEntry("a"_el))->text(), "[a]/a"_el);
        settings.setMessageLimit(el::unit::CpCount{2U});
        REQUIRE_EQUAL(el::log::impl::LogLineFormatter{settings}.format(makeEntry("long"_el))->text(), "[]/"_el);
    }

private:
    [[nodiscard]] static auto makeEntry(el::text::String message) -> el::log::LogEntry {
        return el::log::LogEntry{
            1U,
            el::time::Timestamp{
                el::time::Date::fromYearMonthDay(2026, 8, 31),
                el::time::Time{el::time::Hour{12}, el::time::Minute{34}, el::time::Second{56}}},
            el::log::LogLevel::Warning,
            el::log::LogPath{"app/database/query"_el},
            std::move(message)};
    }
};
