// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../core/ApplicationTestScope.hpp"

#include <erbsland/core/Application.hpp>
#include <erbsland/err/ParameterError.hpp>
#include <erbsland/system/UserLookup.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <memory>

using namespace el::text::literals;

TESTED_TARGETS(UserLookup Application)
class UserLookupTest final : public el::UnitTest {
    class TestBackend final : public el::system::impl::UserLookupBackend {
    public:
        [[nodiscard]] auto userNameForId(const el::system::UserId &id) -> el::system::UserName override {
            ++userLookupCount;
            auto result = el::text::String{"user-"_el};
            result.append(id.value());
            return el::system::UserName{result};
        }

        [[nodiscard]] auto groupNameForId(const el::system::GroupId &id) -> el::system::GroupName override {
            ++groupLookupCount;
            auto result = el::text::String{"group-"_el};
            result.append(id.value());
            return el::system::GroupName{result};
        }

        [[nodiscard]] auto userIdForName(const el::system::UserName &name) -> el::system::UserId override {
            ++userReverseLookupCount;
            auto result = el::text::String{"id-"_el};
            result.append(name.toString());
            return el::system::UserId{result};
        }

        [[nodiscard]] auto groupIdForName(const el::system::GroupName &name) -> el::system::GroupId override {
            ++groupReverseLookupCount;
            auto result = el::text::String{"id-"_el};
            result.append(name.toString());
            return el::system::GroupId{result};
        }

        int userLookupCount{};
        int groupLookupCount{};
        int userReverseLookupCount{};
        int groupReverseLookupCount{};
    };

public:
    void testCachesUserAndGroupNames() {
        auto backend = std::make_unique<TestBackend>();
        auto *backendPtr = backend.get();
        auto lookup = el::system::UserLookup{std::move(backend)};

        REQUIRE_EQUAL(lookup.userNameForId(el::system::UserId{"42"_el}).toString(), el::text::String{"user-42"_el});
        REQUIRE_EQUAL(lookup.userNameForId(el::system::UserId{"42"_el}).toString(), el::text::String{"user-42"_el});
        REQUIRE_EQUAL(backendPtr->userLookupCount, 1);

        REQUIRE_EQUAL(lookup.groupNameForId(el::system::GroupId{"7"_el}).toString(), el::text::String{"group-7"_el});
        REQUIRE_EQUAL(lookup.groupNameForId(el::system::GroupId{"7"_el}).toString(), el::text::String{"group-7"_el});
        REQUIRE_EQUAL(backendPtr->groupLookupCount, 1);

        REQUIRE_EQUAL(
            lookup.userIdForName(el::system::UserName{"seven"_el}).toString(), el::text::String{"id-seven"_el});
        REQUIRE_EQUAL(
            lookup.userIdForName(el::system::UserName{"seven"_el}).toString(), el::text::String{"id-seven"_el});
        REQUIRE_EQUAL(backendPtr->userReverseLookupCount, 1);

        REQUIRE_EQUAL(
            lookup.groupIdForName(el::system::GroupName{"staff"_el}).toString(), el::text::String{"id-staff"_el});
        REQUIRE_EQUAL(
            lookup.groupIdForName(el::system::GroupName{"staff"_el}).toString(), el::text::String{"id-staff"_el});
        REQUIRE_EQUAL(backendPtr->groupReverseLookupCount, 1);

        lookup.clearCache();
        REQUIRE_EQUAL(lookup.userNameForId(el::system::UserId{"42"_el}).toString(), el::text::String{"user-42"_el});
        REQUIRE_EQUAL(backendPtr->userLookupCount, 2);
    }

    void testRejectsEmptyIdentifiers() {
        auto lookup = el::system::UserLookup{std::make_unique<TestBackend>()};

        REQUIRE_THROWS_AS(el::err::ParameterError, lookup.userNameForId(el::system::UserId{}));
        REQUIRE_THROWS_AS(el::err::ParameterError, lookup.groupNameForId(el::system::GroupId{}));
        REQUIRE_THROWS_AS(el::err::ParameterError, lookup.userIdForName(el::system::UserName{}));
        REQUIRE_THROWS_AS(el::err::ParameterError, lookup.groupIdForName(el::system::GroupName{}));
    }

    void testNameFormattingWithDomain() {
        const auto userName = el::system::UserName::fromString("DOMAIN\\ada"_el);
        REQUIRE_EQUAL(userName.domain(), el::text::String{"DOMAIN"_el});
        REQUIRE_EQUAL(userName.name(), el::text::String{"ada"_el});
        REQUIRE_EQUAL(userName.toString(), el::text::String{"DOMAIN\\ada"_el});

        const auto groupName = el::system::GroupName::fromString("DOMAIN\\staff"_el);
        REQUIRE_EQUAL(groupName.domain(), el::text::String{"DOMAIN"_el});
        REQUIRE_EQUAL(groupName.name(), el::text::String{"staff"_el});
        REQUIRE_EQUAL(groupName.toString(), el::text::String{"DOMAIN\\staff"_el});
    }

    void testApplicationProvidesSharedUserLookup() {
        auto scope = ApplicationTestScope<>{};

        auto &first = scope.app().userLookup();
        auto &second = scope.app().userLookup();
        REQUIRE(&first == &second);
    }
};
