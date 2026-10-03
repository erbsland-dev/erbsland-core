// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/Literals.hpp>
#include <erbsland/LogWriterFilter.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace el::text::literals;

TESTED_TARGETS(LogWriterFilter)
class LogWriterFilterTest final : public el::UnitTest {
public:
    void testDefaultAcceptanceAndExplicitInclusion() {
        auto filter = el::log::LogWriterFilter{};
        const auto path = el::log::LogPath{"app/query"_el};
        REQUIRE_EQUAL(filter.acceptedLevels(), el::log::LogLevels{el::log::LogLevel::All});
        REQUIRE(filter.acceptedPaths().empty());
        REQUIRE(filter.accepts(el::log::LogLevel::Trace, path));
        REQUIRE(filter.accepts(el::log::LogLevel::Error, {}));
        REQUIRE(&filter.setAcceptedLevels({el::log::LogLevel::Warning, el::log::LogLevel::Error}) == &filter);
        REQUIRE_FALSE(filter.accepts(el::log::LogLevel::Trace, path));
        REQUIRE_FALSE(filter.accepts(el::log::LogLevel::Information, path));
        REQUIRE(filter.accepts(el::log::LogLevel::Warning, path));
        REQUIRE(filter.accepts(el::log::LogLevel::Error, path));
    }

    void testRejectedLevelsAndLastSetterWins() {
        auto filter = el::log::LogWriterFilter{};
        filter.setAcceptedLevels(el::log::LogLevel::Error);
        REQUIRE(&filter.setRejectedLevels({el::log::LogLevel::Trace, el::log::LogLevel::Warning}) == &filter);
        REQUIRE_FALSE(filter.accepts(el::log::LogLevel::Trace, {}));
        REQUIRE(filter.accepts(el::log::LogLevel::Information, {}));
        REQUIRE_FALSE(filter.accepts(el::log::LogLevel::Warning, {}));
        REQUIRE(filter.accepts(el::log::LogLevel::Error, {}));
        REQUIRE_EQUAL(
            filter.acceptedLevels(), el::log::LogLevels({el::log::LogLevel::Information, el::log::LogLevel::Error}));
        filter.setAcceptedLevels(el::log::LogLevel::Trace);
        REQUIRE(filter.accepts(el::log::LogLevel::Trace, {}));
        REQUIRE_FALSE(filter.accepts(el::log::LogLevel::Information, {}));
        REQUIRE_FALSE(filter.accepts(el::log::LogLevel::Error, {}));
    }

    void testEmptyAndAllLevelSelections() {
        auto filter = el::log::LogWriterFilter{};
        filter.setAcceptedLevels({});
        REQUIRE(filter.acceptedLevels().isEmpty());
        REQUIRE_FALSE(filter.accepts(el::log::LogLevel::Trace, {}));
        REQUIRE_FALSE(filter.accepts(el::log::LogLevel::Error, {}));
        filter.setRejectedLevels({});
        REQUIRE_EQUAL(filter.acceptedLevels(), el::log::LogLevels{el::log::LogLevel::All});
        filter.setRejectedLevels(el::log::LogLevel::All);
        REQUIRE(filter.acceptedLevels().isEmpty());
        filter.setAcceptedLevels(el::log::LogLevel::All);
        REQUIRE(filter.accepts(el::log::LogLevel::Trace, {}));
        REQUIRE(filter.accepts(el::log::LogLevel::Error, {}));
    }

    void testAcceptedPathsCombineWithLevelsAndCopies() {
        auto filter = el::log::LogWriterFilter{};
        REQUIRE(&filter.addAcceptedPath(el::log::LogPath{"app"_el}) == &filter);
        filter.setRejectedLevels(el::log::LogLevel::Trace).addAcceptedPath(el::log::LogPath{"worker"_el});
        REQUIRE_EQUAL(filter.acceptedPaths().size(), std::size_t{2U});
        REQUIRE(filter.accepts(el::log::LogLevel::Warning, el::log::LogPath{"app"_el}));
        REQUIRE(filter.accepts(el::log::LogLevel::Error, el::log::LogPath{"app/query"_el}));
        REQUIRE(filter.accepts(el::log::LogLevel::Information, el::log::LogPath{"worker/job"_el}));
        REQUIRE_FALSE(filter.accepts(el::log::LogLevel::Information, el::log::LogPath{"application"_el}));
        REQUIRE_FALSE(filter.accepts(el::log::LogLevel::Trace, el::log::LogPath{"app/query"_el}));
        REQUIRE_FALSE(filter.accepts(el::log::LogLevel::Error, {}));
        const auto copied = filter;
        filter.setAcceptedLevels(el::log::LogLevel::Trace).addAcceptedPath({});
        REQUIRE(filter.accepts(el::log::LogLevel::Trace, el::log::LogPath{"other"_el}));
        REQUIRE_FALSE(copied.accepts(el::log::LogLevel::Trace, el::log::LogPath{"other"_el}));
        REQUIRE_FALSE(copied.accepts(el::log::LogLevel::Error, el::log::LogPath{"other"_el}));
        REQUIRE(copied.accepts(el::log::LogLevel::Error, el::log::LogPath{"app/query"_el}));
    }
};
