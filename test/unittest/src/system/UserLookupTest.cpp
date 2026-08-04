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
            auto result = el::text::StringEditor{"user-"_el};
            result.append(id.value());
            return el::system::UserName{result};
        }

        [[nodiscard]] auto groupNameForId(const el::system::GroupId &id) -> el::system::GroupName override {
            ++groupLookupCount;
            auto result = el::text::StringEditor{"group-"_el};
            result.append(id.value());
            return el::system::GroupName{result};
        }

        [[nodiscard]] auto userIdForName(const el::system::UserName &name) -> el::system::UserId override {
            ++userReverseLookupCount;
            auto result = el::text::StringEditor{"id-"_el};
            result.append(name.toString());
            return el::system::UserId{result};
        }

        [[nodiscard]] auto groupIdForName(const el::system::GroupName &name) -> el::system::GroupId override {
            ++groupReverseLookupCount;
            auto result = el::text::StringEditor{"id-"_el};
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

        const auto firstUserName = lookup.userNameForId(el::system::UserId{"42"_el});
        REQUIRE_EQUAL(firstUserName.toString(), el::text::StringEditor{"user-42"_el});
        const auto cachedUserName = lookup.userNameForId(el::system::UserId{"42"_el});
        REQUIRE_EQUAL(cachedUserName.toString(), el::text::StringEditor{"user-42"_el});
        REQUIRE_EQUAL(backendPtr->userLookupCount, 1);

        const auto firstGroupName = lookup.groupNameForId(el::system::GroupId{"7"_el});
        REQUIRE_EQUAL(firstGroupName.toString(), el::text::StringEditor{"group-7"_el});
        const auto cachedGroupName = lookup.groupNameForId(el::system::GroupId{"7"_el});
        REQUIRE_EQUAL(cachedGroupName.toString(), el::text::StringEditor{"group-7"_el});
        REQUIRE_EQUAL(backendPtr->groupLookupCount, 1);

        const auto firstUserId = lookup.userIdForName(el::system::UserName{"seven"_el});
        REQUIRE_EQUAL(firstUserId.toString(), el::text::StringEditor{"id-seven"_el});
        const auto cachedUserId = lookup.userIdForName(el::system::UserName{"seven"_el});
        REQUIRE_EQUAL(cachedUserId.toString(), el::text::StringEditor{"id-seven"_el});
        REQUIRE_EQUAL(backendPtr->userReverseLookupCount, 1);

        const auto firstGroupId = lookup.groupIdForName(el::system::GroupName{"staff"_el});
        REQUIRE_EQUAL(firstGroupId.toString(), el::text::StringEditor{"id-staff"_el});
        const auto cachedGroupId = lookup.groupIdForName(el::system::GroupName{"staff"_el});
        REQUIRE_EQUAL(cachedGroupId.toString(), el::text::StringEditor{"id-staff"_el});
        REQUIRE_EQUAL(backendPtr->groupReverseLookupCount, 1);

        lookup.clearCache();
        const auto userNameAfterCacheClear = lookup.userNameForId(el::system::UserId{"42"_el});
        REQUIRE_EQUAL(userNameAfterCacheClear.toString(), el::text::StringEditor{"user-42"_el});
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
        REQUIRE_EQUAL(userName.domain(), el::text::StringEditor{"DOMAIN"_el});
        REQUIRE_EQUAL(userName.name(), el::text::StringEditor{"ada"_el});
        REQUIRE_EQUAL(userName.toString(), el::text::StringEditor{"DOMAIN\\ada"_el});

        const auto groupName = el::system::GroupName::fromString("DOMAIN\\staff"_el);
        REQUIRE_EQUAL(groupName.domain(), el::text::StringEditor{"DOMAIN"_el});
        REQUIRE_EQUAL(groupName.name(), el::text::StringEditor{"staff"_el});
        REQUIRE_EQUAL(groupName.toString(), el::text::StringEditor{"DOMAIN\\staff"_el});
    }

    void testApplicationProvidesSharedUserLookup() {
        auto scope = ApplicationTestScope<>{};

        auto &first = scope.app().userLookup();
        auto &second = scope.app().userLookup();
        REQUIRE_EQUAL(&first, &second);
    }
};
