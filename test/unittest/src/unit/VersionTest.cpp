// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/text/String.hpp>
#include <erbsland/unit/all.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <compare>
#include <concepts>
#include <cstdint>
#include <functional>
#include <limits>
#include <type_traits>

using namespace el::unit;

using namespace el::text::literals;

TESTED_TARGETS(
    VersionPart VersionUnit MajorUnit MinorUnit RevisionUnit BuildNumberUnit Major Minor Revision BuildNumber Version
        VersionRange)
class VersionTest final : public el::UnitTest {
public:
    void testCompileTimeContracts() {

        static_assert(std::derived_from<VersionUnit, IntegerUnit>);
        static_assert(std::derived_from<MajorUnit, VersionUnit>);
        static_assert(std::derived_from<MinorUnit, VersionUnit>);
        static_assert(std::derived_from<RevisionUnit, VersionUnit>);
        static_assert(std::derived_from<BuildNumberUnit, VersionUnit>);
        static_assert(std::same_as<Major, IntegerUnitIndex<MajorUnit>>);
        static_assert(std::same_as<Minor, IntegerUnitIndex<MinorUnit>>);
        static_assert(std::same_as<Revision, IntegerUnitIndex<RevisionUnit>>);
        static_assert(std::same_as<BuildNumber, IntegerUnitIndex<BuildNumberUnit>>);
        static_assert(std::same_as<typename Major::Value, uint16_t>);
        static_assert(!Major::cHasNoIndex);
        static_assert(Major::cRawMaximum == std::numeric_limits<uint16_t>::max());
        static_assert(MajorUnit::cPart == VersionPart::Major);
        static_assert(MinorUnit::cPart == VersionPart::Minor);
        static_assert(RevisionUnit::cPart == VersionPart::Revision);
        static_assert(BuildNumberUnit::cPart == VersionPart::Build);

        static_assert(!std::equality_comparable_with<Major, Minor>);
        static_assert(!std::constructible_from<Version, Major, Major>);
        static_assert(!std::constructible_from<Version, Minor, Minor>);
        static_assert(!std::constructible_from<Version, Revision, Revision>);
        static_assert(!std::constructible_from<Version, BuildNumber, BuildNumber>);

        static_assert(Major::zero().isZero());
        static_assert(Major::one().isOne());
        static_assert(Major::minimum().isMinimum());
        static_assert(Major::maximum().isMaximum());
        static_assert(!Major::maximum().isNoIndex());
        static_assert(Major::maximum().incremented().isMaximum());
        static_assert(Major::zero().decremented().isZero());
        static_assert(Major{42}.toRawValue() == 42U);
        static_assert(Major{42}.toSizeT() == 42U);

        static_assert(Version{}.toNumber() == 0U);
        static_assert(Version{1}.major() == Major{1});
        static_assert(Version{1}.minor().isZero());
        static_assert(Version{1, 2}.minor() == Minor{2});
        static_assert(Version{1, 2, 3}.revision() == Revision{3});
        static_assert(Version{1, 2, 3, 4}.build() == BuildNumber{4});
        static_assert(Version{Major{13}, Revision{2}} == Version{13, 0, 2, 0});
        static_assert(Version{BuildNumber{8}, Minor{3}, Major{1}} == Version{1, 3, 0, 8});

        static_assert(Version{1, 2, 3, 4}.toNumber() == 0x0001000200030004ULL);
        static_assert(Version::fromNumber(0x0001000200030004ULL) == Version{1, 2, 3, 4});
        static_assert(Version{1, 2, 3, 4} < Version{1, 2, 3, 5});
        static_assert(Version{1, 2, 3, 4} < Version{1, 2, 4, 0});
        static_assert(
            Version{1, 2, 3, 4}.compare(Version{1, 9, 9, 9}, VersionPart::Major) == std::strong_ordering::equal);
        static_assert(
            Version{1, 2, 3, 4}.compare(Version{1, 2, 9, 9}, VersionPart::Minor) == std::strong_ordering::equal);
        static_assert(
            Version{1, 2, 3, 4}.compare(Version{1, 2, 3, 9}, VersionPart::Revision) == std::strong_ordering::equal);
        static_assert(
            Version{1, 2, 3, 4}.compare(Version{1, 2, 3, 9}, VersionPart::Build) == std::strong_ordering::less);

        static_assert(VersionRange::all().contains(Version{99, 99, 99, 99}));
        static_assert(VersionRange::atLeast(Version{2}).contains(Version{2}));
        static_assert(VersionRange::atMost(Version{2}).contains(Version{2}));
        static_assert(VersionRange::between(Version{1, 2}, Version{2, 0}).contains(Version{1, 9}));
        static_assert(VersionRange::exact(Version{1, 2, 3}).contains(Version{1, 2, 3}));
        static_assert(VersionRange::exact(Version{1, 2, 3}).contains(Version{1, 2, 9}, VersionPart::Minor));
        static_assert(!VersionRange::between(Version{2}, Version{1}).contains(Version{1, 5}));
        static_assert(VersionRange::between(Version{2}, Version{1}).isEmpty());
    }

    void testVersionUnitRuntimeBehavior() {

        auto major = Major{7};
        REQUIRE(major == Major{7});
        REQUIRE((major++ == Major{7}));
        REQUIRE(major == Major{8});
        REQUIRE((++major == Major{9}));
        REQUIRE((major-- == Major{9}));
        REQUIRE(major == Major{8});
        REQUIRE((--major == Major{7}));

        auto maximum = Major::maximum();
        ++maximum;
        REQUIRE(maximum.isMaximum());

        auto minimum = Major::minimum();
        --minimum;
        REQUIRE(minimum.isMinimum());
    }

    void testVersionRuntimeBehavior() {

        auto version = Version{1, 2, 3, 4};
        REQUIRE(version.major() == Major{1});
        REQUIRE(version.minor() == Minor{2});
        REQUIRE(version.revision() == Revision{3});
        REQUIRE(version.build() == BuildNumber{4});

        version.setMajor(Major{5});
        version.setMinor(Minor{6});
        version.setRevision(Revision{7});
        version.setBuild(BuildNumber{8});
        REQUIRE(version == Version{5, 6, 7, 8});

        REQUIRE(Version{1, 2, 3, 4}.compare(Version{1, 2, 3, 9}, VersionPart::Revision) == std::strong_ordering::equal);
        REQUIRE(Version{1, 2, 3, 4}.compare(Version{1, 2, 3, 9}, VersionPart::Build) == std::strong_ordering::less);
        REQUIRE(Version::fromNumber(version.toNumber()) == version);
    }

    void testVersionToString() {

        const auto version = Version{1, 2, 3, 4};

        REQUIRE(version.toString(VersionPart::Major) == "1"_el);
        REQUIRE(version.toString(VersionPart::Minor) == "1.2"_el);
        REQUIRE(version.toString(VersionPart::Revision) == "1.2.3"_el);
        REQUIRE(version.toString(VersionPart::Build) == "1.2.3.4"_el);
        REQUIRE(version.toString() == "1.2.3"_el);
        REQUIRE(Version{}.toString() == "0.0.0"_el);
    }

    void testVersionRangeRuntimeBehavior() {

        const auto all = VersionRange::all();
        REQUIRE_FALSE(all.hasMinimum());
        REQUIRE_FALSE(all.hasMaximum());
        REQUIRE(all.contains(Version{65535, 65535, 65535, 65535}));

        const auto atLeast = VersionRange::atLeast(Version{1, 2});
        REQUIRE(atLeast.hasMinimum());
        REQUIRE_FALSE(atLeast.hasMaximum());
        REQUIRE(atLeast.contains(Version{1, 2}));
        REQUIRE(atLeast.contains(Version{1, 2, 9}, VersionPart::Minor));
        REQUIRE_FALSE(atLeast.contains(Version{1, 1, 9}, VersionPart::Minor));

        const auto atMost = VersionRange::atMost(Version{2});
        REQUIRE_FALSE(atMost.hasMinimum());
        REQUIRE(atMost.hasMaximum());
        REQUIRE(atMost.contains(Version{2}));
        REQUIRE_FALSE(atMost.contains(Version{2, 0, 0, 1}));
        REQUIRE(atMost.contains(Version{2, 9}, VersionPart::Major));

        const auto range = VersionRange::between(Version{1, 2}, Version{1, 4});
        REQUIRE(range.contains(Version{1, 2}));
        REQUIRE(range.contains(Version{1, 3, 9}));
        REQUIRE(range.contains(Version{1, 4}));
        REQUIRE_FALSE(range.contains(Version{1, 4, 0, 1}));
        REQUIRE(range.contains(Version{1, 4, 9, 9}, VersionPart::Minor));
        REQUIRE(Version{1, 3}.inRange(range));

        const auto exact = VersionRange::exact(Version{1, 2, 3});
        REQUIRE(exact.contains(Version{1, 2, 3}));
        REQUIRE_FALSE(exact.contains(Version{1, 2, 3, 1}));
        REQUIRE(exact.contains(Version{1, 2, 3, 1}, VersionPart::Revision));

        const auto empty = VersionRange::between(Version{2}, Version{1});
        REQUIRE(empty.isEmpty());
        REQUIRE_FALSE(empty.contains(Version{1, 5}));
        REQUIRE_FALSE(Version{1, 5}.inRange(empty));
    }

    void testHashSupport() {

        REQUIRE(std::hash<Major>{}(Major{7}) == std::hash<Major>{}(Major{7}));
        REQUIRE(std::hash<Version>{}(Version{1, 2, 3, 4}) == std::hash<Version>{}(Version{1, 2, 3, 4}));
        REQUIRE(
            std::hash<VersionRange>{}(VersionRange::exact(Version{1})) ==
            std::hash<VersionRange>{}(VersionRange::exact(Version{1})));
    }
};
