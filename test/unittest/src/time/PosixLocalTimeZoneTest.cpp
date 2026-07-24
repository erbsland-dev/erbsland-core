// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/text/Literals.hpp>
#include <erbsland/time/tz/impl/PosixLocalTimeZoneBackend.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <cstdlib>
#include <optional>
#include <string>

using namespace el::text::literals;

TESTED_TARGETS(PosixLocalTimeZoneBackend)
class PosixLocalTimeZoneTest final : public el::UnitTest {
    class EnvironmentGuard final {
    public:
        EnvironmentGuard() {
            if (const auto *value = std::getenv("TZ"); value != nullptr) {
                _oldValue = value;
            }
        }

        ~EnvironmentGuard() {
            if (_oldValue.has_value()) {
                static_cast<void>(::setenv("TZ", _oldValue->c_str(), 1));
            } else {
                static_cast<void>(::unsetenv("TZ"));
            }
        }

        // deletions
        EnvironmentGuard(const EnvironmentGuard &) = delete;
        EnvironmentGuard(EnvironmentGuard &&) = delete;
        auto operator=(const EnvironmentGuard &) -> EnvironmentGuard & = delete;
        auto operator=(EnvironmentGuard &&) -> EnvironmentGuard & = delete;

    private:
        std::optional<std::string> _oldValue;
    };

public:
    void testEnvironmentSelection() {
        const auto guard = EnvironmentGuard{};
        static_cast<void>(::setenv("TZ", ":Europe/Zurich", 1));
        auto zone = el::time::tz::impl::PosixLocalTimeZoneBackend{}.timeZone();
        REQUIRE(zone.has_value());
        REQUIRE_EQUAL(zone->name(), "Europe/Zurich"_el);

        static_cast<void>(::setenv("TZ", ":/usr/share/zoneinfo/America/New_York", 1));
        zone = el::time::tz::impl::PosixLocalTimeZoneBackend{}.timeZone();
        REQUIRE(zone.has_value());
        REQUIRE_EQUAL(zone->name(), "America/New_York"_el);

        static_cast<void>(::setenv("TZ", "Europe/Zurich ", 1));
        REQUIRE_FALSE(el::time::tz::impl::PosixLocalTimeZoneBackend{}.timeZone().has_value());
    }
};
