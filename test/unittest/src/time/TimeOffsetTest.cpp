// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/time/all.hpp>
#include <erbsland/time/StdFormat.hpp>
#include <erbsland/time/tz/impl/LocalTimeZoneBackend.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace el::time;

using namespace el::text::literals;

TESTED_TARGETS(time TimeOffset)
class TimeOffsetTest final : public el::UnitTest {
    class Backend final : public el::time::tz::impl::LocalTimeZoneBackend {
    public:
        explicit Backend(std::optional<el::time::TimeZone> result, int *externalCalls = nullptr) :
            _result{result}, _externalCalls{externalCalls} {}
        [[nodiscard]] auto timeZone() noexcept -> std::optional<el::time::TimeZone> override {
            ++calls;
            if (_externalCalls != nullptr) {
                ++*_externalCalls;
            }
            return _result;
        }

        int calls{0};

    private:
        std::optional<el::time::TimeZone> _result;
        int *_externalCalls;
    };

public:
    void testUtcAndFixedOffsets() {

        const auto utc = tz::TimeOffset{};
        REQUIRE(utc.isUtc());
        REQUIRE_FALSE(utc.isStaticOffset());
        REQUIRE_FALSE(utc.isZone());
        REQUIRE_FALSE(utc.isDst());
        REQUIRE_EQUAL(utc.offset(), Seconds{0});

        const auto fixed = tz::TimeOffset{Seconds{19800}};
        REQUIRE_FALSE(fixed.isUtc());
        REQUIRE(fixed.isStaticOffset());
        REQUIRE_FALSE(fixed.isZone());
        REQUIRE_EQUAL(fixed.offset(), Seconds{19800});
    }

    void testNamedOffsetStorage() {

        const auto offset = tz::TimeOffset{Seconds{7200}, true, TimeZoneId{321}, 7};
        REQUIRE_FALSE(offset.isUtc());
        REQUIRE_FALSE(offset.isStaticOffset());
        REQUIRE(offset.isZone());
        REQUIRE(offset.isDst());
        REQUIRE_EQUAL(offset.offset(), Seconds{7200});
        REQUIRE_EQUAL(offset.zoneId(), TimeZoneId{321});
        REQUIRE_EQUAL(offset.abbreviationId(), 7);
    }

    void testLocalBackendPolicy() {
        using el::time::TimeZone;

        auto known = Backend{TimeZone::fromNameOrThrow("Europe/Zurich"_el)};
        auto resolved = el::time::tz::impl::localTimeZoneFromBackend(known);
        REQUIRE_EQUAL(resolved.name(), "Europe/Zurich"_el);
        REQUIRE(resolved.isLocalTime());
        REQUIRE_EQUAL(known.calls, 1);
        auto missing = Backend{std::nullopt};
        const auto fallback = el::time::tz::impl::localTimeZoneFromBackend(missing);
        REQUIRE(fallback.isUtc());
        REQUIRE(fallback.isLocalTime());
        REQUIRE_FALSE(fallback == TimeZone::utc());
    }

    void testLocalTimeIsCached() {

        auto backendCalls = 0;
        auto backend =
            std::make_unique<Backend>(el::time::TimeZone::fromNameOrThrow("Europe/Zurich"_el), &backendCalls);
        const auto cache = el::time::tz::impl::LocalTimeZoneCache{std::move(backend)};
        REQUIRE_EQUAL(backendCalls, 1);
        REQUIRE_EQUAL(cache.value().name(), "Europe/Zurich"_el);
        REQUIRE(cache.value().isLocalTime());
        REQUIRE_EQUAL(cache.value().name(), "Europe/Zurich"_el);
        REQUIRE_EQUAL(backendCalls, 1);

        const auto first = el::time::TimeZone::local();
        const auto second = el::time::TimeZone::local();
        REQUIRE_EQUAL(first, second);
        REQUIRE(first.isLocalTime());
    }
};
