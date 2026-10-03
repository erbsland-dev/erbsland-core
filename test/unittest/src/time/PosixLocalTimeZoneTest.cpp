// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/EnvironmentVariables.hpp>
#include <erbsland/Literals.hpp>
#include <erbsland/time/tz/impl/PosixLocalTimeZoneBackend.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <optional>

using namespace el::text::literals;

TESTED_TARGETS(PosixLocalTimeZoneBackend)
class PosixLocalTimeZoneTest final : public el::UnitTest {
private:
    static constexpr auto cTimeZoneVariable = "TZ"_el;

    class EnvironmentGuard final {
    public:
        EnvironmentGuard() : _oldValue{_environment.get(cTimeZoneVariable)} {}

        ~EnvironmentGuard() {
            if (_oldValue.has_value()) {
                _environment.set(cTimeZoneVariable, *_oldValue);
            } else {
                _environment.remove(cTimeZoneVariable);
            }
        }

        // deletions
        EnvironmentGuard(const EnvironmentGuard &) = delete;
        EnvironmentGuard(EnvironmentGuard &&) = delete;
        auto operator=(const EnvironmentGuard &) -> EnvironmentGuard & = delete;
        auto operator=(EnvironmentGuard &&) -> EnvironmentGuard & = delete;

    private:
        el::system::EnvironmentVariables _environment;
        std::optional<el::text::String> _oldValue;
    };

public:
    void testEnvironmentSelection() {
        const auto guard = EnvironmentGuard{};
        auto environment = el::system::EnvironmentVariables{};
        environment.setOrThrow(cTimeZoneVariable, ":Europe/Zurich"_el);
        auto zone = el::time::tz::impl::PosixLocalTimeZoneBackend{}.detectedTimeZone();
        REQUIRE(zone.has_value());
        REQUIRE_EQUAL(zone->name(), "Europe/Zurich"_el);

        environment.setOrThrow(cTimeZoneVariable, ":/usr/share/zoneinfo/America/New_York"_el);
        zone = el::time::tz::impl::PosixLocalTimeZoneBackend{}.detectedTimeZone();
        REQUIRE(zone.has_value());
        REQUIRE_EQUAL(zone->name(), "America/New_York"_el);

        environment.setOrThrow(cTimeZoneVariable, "Europe/Zurich "_el);
        REQUIRE_FALSE(el::time::tz::impl::PosixLocalTimeZoneBackend{}.detectedTimeZone().has_value());
    }
};
